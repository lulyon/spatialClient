/// @file layerMetadata.cc
/// @author luliang@ict.ac.cn
/// @copybrief Copyright 2012 ICT, CAS. All rights reserved.
/// @version 0.6
/// @date 2013-06-19

#include "layerMetadata.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <ogrsf_frmts.h>

LayerMetadata::LayerMetadata() :
		metadatalength_(0), layernamelength_(0), layername_(NULL), geotype_(0), strWKTlength_(
				0), strWKT_(NULL), buffer_(NULL), bufferflag_(UNINITIALIZED) {
}

LayerMetadata::LayerMetadata(const LayerMetadata & metadata) :
		metadatalength_(0), layernamelength_(0), layername_(NULL), geotype_(0), strWKTlength_(
				0), strWKT_(NULL), buffer_(NULL), bufferflag_(UNINITIALIZED) {
	setMetadata(metadata);
}

LayerMetadata::LayerMetadata(OGRLayer *layer) :
		metadatalength_(0), layernamelength_(0), layername_(NULL), geotype_(0), strWKTlength_(
				0), strWKT_(NULL), buffer_(NULL), bufferflag_(UNINITIALIZED) {
	setMetadata(layer);
}

LayerMetadata::LayerMetadata(const char * bytes) :
		metadatalength_(0), layernamelength_(0), layername_(NULL), geotype_(0), strWKTlength_(
				0), strWKT_(NULL), buffer_(NULL), bufferflag_(UNINITIALIZED) {
	setMetadata(bytes);
}

LayerMetadata::~LayerMetadata() {
	if (layername_)
		free(layername_);
	if (strWKT_)
		free(strWKT_);
	if (buffer_)
		free(buffer_);
}

const char *LayerMetadata::getBytes() {

	// alloc memory or return the buffered result.
	if (bufferflag_ == UNINITIALIZED) {
		buffer_ = (char *) malloc(metadatalength_);
	} else if (bufferflag_ == STALE) {
		buffer_ = (char *) realloc(buffer_, metadatalength_);
	} else {
		return buffer_;
	}
	if (buffer_ == NULL) {
		fprintf(stderr, "Fail to alloc memory for buffer_.\n");
		return NULL;
	}

	char *bytes = buffer_;
	// serialize members to buffer_
	int offset = 0;
	//metadatalength_
	memcpy(bytes + offset, &metadatalength_, sizeof(metadatalength_));
	offset += sizeof(metadatalength_);

	// layername_ and layernamelength_
	memcpy(bytes + offset, &layernamelength_, sizeof(layernamelength_));
	offset += sizeof(layernamelength_);
	memcpy(bytes + offset, layername_, layernamelength_);
	offset += layernamelength_;

	// geotype_
	memcpy(bytes + offset, &geotype_, sizeof(geotype_));
	offset += sizeof(geotype_);

	// strWKT_ and strWKTlength_
	memcpy(bytes + offset, &strWKTlength_, sizeof(strWKTlength_));
	offset += sizeof(strWKTlength_);
	memcpy(bytes + offset, strWKT_, strWKTlength_);
	offset += strWKTlength_;
	assert(offset == layernamelength_);

	bufferflag_ = LATEST;
	return buffer_;
}

int LayerMetadata::getMetadataLength() const {
	return metadatalength_;
}

int LayerMetadata::getLayernameLength() const {
	return layernamelength_;
}

const char *LayerMetadata::getLayername() const {
	return layername_;
}

int LayerMetadata::getGeotype() const {
	return geotype_;
}

int LayerMetadata::getStrWKTlength() const {
	return strWKTlength_;
}

const char *LayerMetadata::getStrWKT() const {
	return strWKT_;
}

void LayerMetadata::setMetadata(OGRLayer *layer) {
	if (layer == NULL)
		return;

	metadatalength_ = 0;
	// metadatalength_
	metadatalength_ += sizeof(metadatalength_);

	// layername_ and layernamelength_
	const char *layername = layer->GetName();
	layernamelength_ = strlen(layername) + 1;
	if (layername_ == NULL) {
		layername_ = (char *) malloc(layernamelength_);
	} else {
		layername_ = (char *) realloc(layername_, layernamelength_);
	}
	if (layername_ == NULL) {
		fprintf(stderr, "Fail to alloc memory for layername_.\n");
		return;
	}
	memcpy(layername_, layername, layernamelength_);
	metadatalength_ += sizeof(layernamelength_) + layernamelength_;

	// geotype_
	geotype_ = (int) layer->GetGeomType();
	metadatalength_ += sizeof(geotype_);

	// strWKT_ and strWKTlength_
	if(strWKT_) free(strWKT_);
	strWKT_ = NULL;
	OGRSpatialReference *poSR = layer->GetSpatialRef();
	if (poSR) {
		poSR->exportToWkt(&strWKT_);
	} else {
		fprintf(stdout, "since no srs specified,default would be assigned.");
		strWKT_ = "";
	}
	strWKTlength_ = strlen(strWKT_) + 1;
	metadatalength_ += strWKTlength_ + sizeof(strWKTlength_);

	// set buffer flag.
	if (bufferflag_ == LATEST)
		bufferflag_ = STALE;
}
void LayerMetadata::setMetadata(const char * bytes) {
	if (bytes == NULL)
		return;

	int offset = 0;
	//metadatalength_
	memcpy(&metadatalength_, bytes + offset, sizeof(metadatalength_));
	offset += sizeof(metadatalength_);

	// layername_ and layernamelength_
	memcpy(&layernamelength_, bytes + offset, sizeof(layernamelength_));
	offset += sizeof(layernamelength_);
	if (layername_ == NULL) {
		layername_ = (char *) malloc(layernamelength_);
	} else {
		layername_ = (char *) realloc(layername_, layernamelength_);
	}
	if (layername_ == NULL) {
		fprintf(stderr, "Fail to alloc memory for layername_.\n");
		return;
	}
	memcpy(layername_, bytes + offset, layernamelength_);
	offset += layernamelength_;

	// geometry type
	memcpy(&geotype_, bytes + offset, sizeof(geotype_));
	offset += sizeof(geotype_);

	// strWKT georeference
	memcpy(&strWKTlength_, bytes + offset, sizeof(strWKTlength_));
	offset += sizeof(strWKTlength_);
	if (strWKT_ == NULL) {
		strWKT_ = (char *) malloc(strWKTlength_);
	} else {
		strWKT_ = (char *) realloc(strWKT_, strWKTlength_);
	}
	if (strWKT_ == NULL) {
		fprintf(stderr, "Fail to alloc memory for strWKT_.\n");
		return;
	}
	memcpy(strWKT_, bytes + offset, strWKTlength_);
	offset += strWKTlength_;

	assert(offset == metadatalength_);

	// alloc memory for buffer_
	if (bufferflag_ == UNINITIALIZED) {
		buffer_ = (char *) malloc(metadatalength_);
	} else {
		buffer_ = (char *) realloc(buffer_, metadatalength_);
	}
	if (buffer_ == NULL) {
		fprintf(stderr, "Fail to alloc memory for buffer_.\n");
		return;
	}

	memcpy(buffer_, bytes, metadatalength_);

	// set buffer flag.
	bufferflag_ = LATEST;
}

void LayerMetadata::setMetadata(const LayerMetadata &metadata) {
	// metadatalength_
	metadatalength_ = metadata.getMetadataLength();

	// layername_
	layernamelength_ = metadata.getLayernameLength();
	if (layername_ == NULL) {
		layername_ = (char *) malloc(layernamelength_);
	} else {
		layername_ = (char *) realloc(layername_, layernamelength_);
	}
	if (layername_ == NULL) {
		fprintf(stderr, "Fail to alloc memory for layername_.\n");
		return;
	}
	memcpy(layername_, metadata.getLayername(), layernamelength_);

	// geotype_
	geotype_ = metadata.getGeotype();

	// strWKT_
	strWKTlength_ = metadata.getStrWKTlength();
	if (strWKT_ == NULL) {
		strWKT_ = (char *) malloc(strWKTlength_);
	} else {
		strWKT_ = (char *) realloc(strWKT_, strWKTlength_);
	}
	if (strWKT_ == NULL) {
		fprintf(stderr, "Fail to alloc memory for strWKT_.\n");
		return;
	}
	memcpy(strWKT_, metadata.getStrWKT(), strWKTlength_);

	// set buffer flag.
	if (bufferflag_ == LATEST)
		bufferflag_ = STALE;
}

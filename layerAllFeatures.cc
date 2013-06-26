/// @file layerAllFeatures.cc
/// @author luliang@ict.ac.cn
/// @copybrief Copyright 2012 ICT, CAS. All rights reserved.
/// @version 0.1
/// @date 2013-06-25

#include "layerAllFeatures.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <ogrsf_frmts.h>

LayerAllFeatures::LayerAllFeatures() :
		featurelength_(0), featurecount_(0), features_(NULL), buffer_(NULL), bufferflag_(
				UNINITIALIZED) {
}

LayerAllFeatures::LayerAllFeatures(const LayerAllFeatures & allfeatures) :
		featurelength_(0), featurecount_(0), features_(NULL), buffer_(NULL), bufferflag_(
				UNINITIALIZED) {
	setAllFeatures(allfeatures);
}

LayerAllFeatures::LayerAllFeatures(OGRLayer *layer) :
		featurelength_(0), featurecount_(0), features_(NULL), buffer_(NULL), bufferflag_(
				UNINITIALIZED) {
	setAllFeatures(layer);
}

LayerAllFeatures::LayerAllFeatures(const char * bytes) :
		featurelength_(0), featurecount_(0), features_(NULL), buffer_(NULL), bufferflag_(
				UNINITIALIZED) {
	setAllFeatures(bytes);
}

LayerAllFeatures::~LayerAllFeatures() {
	for (int i = 0; i < featurecount_; ++i) {
		if (features_[i].wkbbytes_)
			free(features_[i].wkbbytes_);
	}
	if (features_)
		free(features_);
	if (buffer_)
		free(buffer_);
}

void LayerAllFeatures::setAllFeatures(OGRLayer *layer) {
	if (layer == NULL)
		return;
	featurelength_ = 0;
	// featurelength_
	featurelength_ += sizeof(featurelength_);
	// featurecount_
	for (int i = 0; i < featurecount_; ++i) {
		if (features_[i].wkbbytes_)
			free(features_[i].wkbbytes_);
	}

	layer->ResetReading();
	for (OGRFeature *feature = layer->GetNextFeature(); feature != NULL;
			feature = layer->GetNextFeature()) {
		OGRGeometry *geometry = feature->GetGeometryRef();
		if (geometry) {
			// int geometrytype = (int)geometry->getGeometryType();
			featurelength_ += sizeof(int);
			int wkbsize = geometry->WkbSize();
			featurelength_ += sizeof(wkbsize) + wkbsize;

			++featurecount_;
		}
		OGRFeature::DestroyFeature(feature);
	}

	if (features_ == NULL) {
		features_ = (LayerFeature *) malloc(
				sizeof(LayerFeature) * featurecount_);
	} else {
		features_ = (LayerFeature *) realloc(features_,
				sizeof(LayerFeature) * featurecount_);
	}
	if (features_ == NULL) {
		fprintf(stderr, "Fail to alloc memory for features.\n");
		return;
	}

	layer->ResetReading();
	int ifeature = 0;
	for (OGRFeature *feature = layer->GetNextFeature(); feature != NULL;
			feature = layer->GetNextFeature()) {
		OGRGeometry *geometry = feature->GetGeometryRef();
		if (geometry) {
			features_[ifeature].geometrytype_ =
					(int) geometry->getGeometryType();
			features_[ifeature].wkbsize_ = geometry->WkbSize();

			features_[ifeature].wkbbytes_ = (char *) malloc(
					features_[ifeature].wkbsize_);
			if (features_[ifeature].wkbbytes_ == NULL) {
				fprintf(stderr, "Fail to alloc memory for feature wkbbytes.\n");
				return;
			}
			geometry->exportToWkb((OGRwkbByteOrder) wkbNDR,
					(unsigned char *) (features_[ifeature].wkbbytes_));

			++ifeature;
		}
		OGRFeature::DestroyFeature(feature);
	}

	// set buffer flag.
	if (bufferflag_ == LATEST)
		bufferflag_ = STALE;
}

void LayerAllFeatures::setAllFeatures(const char * bytes) {
	if (bytes == NULL)
		return;

	int offset = 0;
	// featurelength_
	memcpy(&featurelength_, bytes + offset, sizeof(featurelength_));
	offset += sizeof(featurelength_);

	// featurecount_
	for (int i = 0; i < featurecount_; ++i) {
		if (features_[i].wkbbytes_)
			free(features_[i].wkbbytes_);
	}
	memcpy(&featurecount_, bytes + offset, sizeof(featurecount_));
	offset += sizeof(featurecount_);

	if (features_ == NULL) {
		features_ = (LayerFeature *) malloc(
				sizeof(LayerFeature) * featurecount_);
	} else {
		features_ = (LayerFeature *) realloc(features_,
				sizeof(LayerFeature) * featurecount_);
	}
	if (features_ == NULL) {
		fprintf(stderr, "Fail to alloc memory for features.\n");
		return;
	}

	for (int ifeature = 0; ifeature < featurecount_; ++ifeature) {
		// geometrytype_
		memcpy(&features_[ifeature].geometrytype_, bytes + offset,
				sizeof(features_[ifeature].geometrytype_));
		offset += sizeof(features_[ifeature].geometrytype_);

		// wkbsize_
		memcpy(&features_[ifeature].wkbsize_, bytes + offset,
				sizeof(features_[ifeature].wkbsize_));
		offset += sizeof(features_[ifeature].wkbsize_);

		// wkbbytes_
		features_[ifeature].wkbbytes_ = (char *) malloc(
				features_[ifeature].wkbsize_);
		if (features_[ifeature].wkbbytes_ == NULL) {
			fprintf(stderr, "Fail to alloc memory for feature wkbbytes.\n");
			return;
		}
		memcpy(features_[ifeature].wkbbytes_, bytes + offset,
				features_[ifeature].wkbsize_);
		offset += features_[ifeature].wkbsize_;
	}

	assert(offset == featurelength_);
	// alloc memory for buffer_
	if (bufferflag_ == UNINITIALIZED) {
		buffer_ = (char *) malloc(featurelength_);
	} else {
		buffer_ = (char *) realloc(buffer_, featurelength_);
	}
	if (buffer_ == NULL) {
		fprintf(stderr, "Fail to alloc memory for buffer_.\n");
		return;
	}

	memcpy(buffer_, bytes, featurelength_);

	// set buffer flag.
	bufferflag_ = LATEST;
}

void LayerAllFeatures::setAllFeatures(const LayerAllFeatures & allfeatures) {
	// featurelength_
	featurelength_ = allfeatures.getFeatureLength();

	// featurecount_
	for (int i = 0; i < featurecount_; ++i) {
		if (features_[i].wkbbytes_)
			free(features_[i].wkbbytes_);
	}
	featurecount_ = allfeatures.getFeatureCount();

	if (features_ == NULL) {
		features_ = (LayerFeature *) malloc(
				sizeof(LayerFeature) * featurecount_);
	} else {
		features_ = (LayerFeature *) realloc(features_,
				sizeof(LayerFeature) * featurecount_);
	}
	if (features_ == NULL) {
		fprintf(stderr, "Fail to alloc memory for features.\n");
		return;
	}

	for (int ifeature = 0; ifeature < featurecount_; ++ifeature) {
		const LayerFeature *feature = allfeatures.getFeature(ifeature);

		// geometrytype_
		features_[ifeature].geometrytype_ = feature->geometrytype_;
		// wkbsize_
		features_[ifeature].wkbsize_ = feature->wkbsize_;

		// wkbbytes_
		features_[ifeature].wkbbytes_ = (char *) malloc(
				features_[ifeature].wkbsize_);
		if (features_[ifeature].wkbbytes_ == NULL) {
			fprintf(stderr, "Fail to alloc memory for feature wkbbytes.\n");
			return;
		}
		memcpy(features_[ifeature].wkbbytes_, feature->wkbbytes_,
				features_[ifeature].wkbsize_);
	}
	// set buffer flag.
	if (bufferflag_ == LATEST)
		bufferflag_ = STALE;
}

const char *LayerAllFeatures::getBytes() {
	// alloc memory or return the buffered result.
	if (bufferflag_ == UNINITIALIZED) {
		buffer_ = (char *) malloc(featurelength_);
	} else if (bufferflag_ == STALE) {
		buffer_ = (char *) realloc(buffer_, featurelength_);
	} else {
		return buffer_;
	}

	char *bytes = buffer_;
	int offset = 0;
	// attrdeflength_
	memcpy(bytes + offset, &featurelength_, sizeof(featurelength_));
	offset += sizeof(featurelength_);

	memcpy(bytes + offset, &featurecount_, sizeof(featurecount_));
	offset += sizeof(featurecount_);

	for (int i = 0; i < featurecount_; i++) {
		// geometrytype
		memcpy(bytes + offset, &features_[i].geometrytype_,
				sizeof(features_[i].geometrytype_));
		offset += sizeof(features_[i].geometrytype_);

		//wkbsize
		memcpy(bytes + offset, &features_[i].wkbsize_,
				sizeof(features_[i].wkbsize_));
		offset += sizeof(features_[i].wkbsize_);

		memcpy(bytes + offset, features_[i].wkbbytes_, features_[i].wkbsize_);
		offset += features_[i].wkbsize_;
	}

	assert(offset == featurelength_);

	bufferflag_ = LATEST;
	return buffer_;

}

int LayerAllFeatures::getFeatureLength() const {
	return featurelength_;
}

int LayerAllFeatures::getFeatureCount() const {
	return featurecount_;
}

const LayerFeature *LayerAllFeatures::getFeatures() const {
	return features_;
}

const LayerFeature *LayerAllFeatures::getFeature(int index) const {
	if (features_ == NULL || index < 0 || index >= featurecount_)
		return NULL;
	return &features_[index];
}

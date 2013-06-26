/// @file layerAttrDef.cc
/// @author luliang@ict.ac.cn
/// @copybrief Copyright 2012 ICT, CAS. All rights reserved.
/// @version 0.1
/// @date 2013-06-21

#include "layerAttrDef.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <ogrsf_frmts.h>

LayerAttrDef::LayerAttrDef() :
		attrdeflength_(0), fieldcount_(0), fields_(NULL), buffer_(NULL), bufferflag_(
				UNINITIALIZED) {
}

LayerAttrDef::LayerAttrDef(const LayerAttrDef & attrdef) :
		attrdeflength_(0), fieldcount_(0), fields_(NULL), buffer_(NULL), bufferflag_(
				UNINITIALIZED) {
	setAttrDef(attrdef);
}

LayerAttrDef::LayerAttrDef(OGRLayer *layer) :
		attrdeflength_(0), fieldcount_(0), fields_(NULL), buffer_(NULL), bufferflag_(
				UNINITIALIZED) {
	setAttrDef(layer);
}

LayerAttrDef::LayerAttrDef(const char * bytes) :
		attrdeflength_(0), fieldcount_(0), fields_(NULL), buffer_(NULL), bufferflag_(
				UNINITIALIZED) {
	setAttrDef(bytes);
}

LayerAttrDef::~LayerAttrDef() {
	for (int i = 0; i < fieldcount_; ++i) {
		if (fields_[i].sztitle_)
			free(fields_[i].sztitle_);
	}
	if (fields_)
		free(fields_);
	if (buffer_)
		free(buffer_);
}

void LayerAttrDef::setAttrDef(OGRLayer *layer) {
	if (layer == NULL)
		return;
	attrdeflength_ = 0;
	// attrdeflength_
	attrdeflength_ += sizeof(attrdeflength_);
	// fieldcount_
	for (int i = 0; i < fieldcount_; ++i) {
		if (fields_[i].sztitle_)
			free(fields_[i].sztitle_);
	}
	fieldcount_ = layer->GetLayerDefn()->GetFieldCount();
	attrdeflength_ += sizeof(fieldcount_);

	if (fields_ == NULL) {
		fields_ = (LayerAttrDefField *) malloc(
				sizeof(LayerAttrDefField) * fieldcount_);
	} else {
		fields_ = (LayerAttrDefField *) realloc(fields_,
				sizeof(LayerAttrDefField) * fieldcount_);
	}
	if (fields_ == NULL) {
		fprintf(stderr, "Fail to alloc memory for fields.\n");
		return;
	}

	for (int ipoField = 0; ipoField < fieldcount_; ipoField++) {
		OGRFieldDefn* poField = layer->GetLayerDefn()->GetFieldDefn(ipoField);
		const char *sztitle = poField->GetNameRef();
		int sztitlelength = strlen(sztitle) + 1;
		attrdeflength_ += sztitlelength + sizeof(sztitlelength);

		fields_[ipoField].sztitlelength_ = sztitlelength;

		fields_[ipoField].sztitle_ = (char *) malloc(
				fields_[ipoField].sztitlelength_);
		if (fields_[ipoField].sztitle_ == NULL) {
			fprintf(stderr, "Fail to alloc memory for field sztitle.\n");
			return;
		}
		memcpy(fields_[ipoField].sztitle_, sztitle,
				fields_[ipoField].sztitlelength_);

		int nWidth = poField->GetWidth();
		attrdeflength_ += sizeof(nWidth);
		fields_[ipoField].nWidth_ = nWidth;

		int nDecimals = poField->GetPrecision();
		attrdeflength_ += sizeof(nDecimals);
		fields_[ipoField].nDecimals_ = nDecimals;

		char fieldtype = (char) poField->GetType();
		attrdeflength_ += sizeof(fieldtype);
		fields_[ipoField].fieldtype_ = fieldtype;
	}

	// set buffer flag.
	if (bufferflag_ == LATEST)
		bufferflag_ = STALE;
}

void LayerAttrDef::setAttrDef(const char * bytes) {
	if (bytes == NULL)
		return;

	int offset = 0;
	// attrdeflength_
	memcpy(&attrdeflength_, bytes + offset, sizeof(attrdeflength_));
	offset += sizeof(attrdeflength_);

	// fieldcount_
	for (int i = 0; i < fieldcount_; ++i) {
		if (fields_[i].sztitle_)
			free(fields_[i].sztitle_);
	}
	memcpy(&fieldcount_, bytes + offset, sizeof(fieldcount_));
	offset += sizeof(fieldcount_);

	if (fields_ == NULL) {
		fields_ = (LayerAttrDefField *) malloc(
				sizeof(LayerAttrDefField) * fieldcount_);
	} else {
		fields_ = (LayerAttrDefField *) realloc(fields_,
				sizeof(LayerAttrDefField) * fieldcount_);
	}
	if (fields_ == NULL) {
		fprintf(stderr, "Fail to alloc memory for fields.\n");
		return;
	}

	for (int ipoField = 0; ipoField < fieldcount_; ipoField++) {
		// sztitle
		memcpy(&fields_[ipoField].sztitlelength_, bytes + offset,
				sizeof(fields_[ipoField].sztitlelength_));

		fields_[ipoField].sztitle_ = (char *) malloc(
				fields_[ipoField].sztitlelength_);
		if (fields_[ipoField].sztitle_ == NULL) {
			fprintf(stderr, "Fail to alloc memory for field sztitle.\n");
			return;
		}
		memcpy(fields_[ipoField].sztitle_, bytes + offset,
				fields_[ipoField].sztitlelength_);

		offset += fields_[ipoField].sztitlelength_
				+ sizeof(fields_[ipoField].sztitlelength_);

		// nWidth
		memcpy(&fields_[ipoField].nWidth_, bytes + offset,
				sizeof(fields_[ipoField].nWidth_));
		offset += sizeof(fields_[ipoField].nWidth_);

		// nDecimals precision
		memcpy(&fields_[ipoField].nDecimals_, bytes + offset,
				sizeof(fields_[ipoField].nDecimals_));
		offset += sizeof(fields_[ipoField].nDecimals_);

		// fieldtype
		memcpy(&fields_[ipoField].fieldtype_, bytes + offset,
				sizeof(fields_[ipoField].fieldtype_));
		offset += sizeof(fields_[ipoField].fieldtype_);
	}

	assert(offset == attrdeflength_);
	// alloc memory for buffer_
	if (bufferflag_ == UNINITIALIZED) {
		buffer_ = (char *) malloc(attrdeflength_);
	} else {
		buffer_ = (char *) realloc(buffer_, attrdeflength_);
	}
	if (buffer_ == NULL) {
		fprintf(stderr, "Fail to alloc memory for buffer_.\n");
		return;
	}

	memcpy(buffer_, bytes, attrdeflength_);

	// set buffer flag.
	bufferflag_ = LATEST;
}

void LayerAttrDef::setAttrDef(const LayerAttrDef & attrdef) {
	// attrdeflength_
	attrdeflength_ = attrdef.getAttrDefLength();

	//fieldcount_
	for (int i = 0; i < fieldcount_; ++i) {
		if (fields_[i].sztitle_)
			free(fields_[i].sztitle_);
	}
	fieldcount_ = attrdef.getFieldCount();

	if (fields_ == NULL) {
		fields_ = (LayerAttrDefField *) malloc(
				sizeof(LayerAttrDefField) * fieldcount_);
	} else {
		fields_ = (LayerAttrDefField *) realloc(fields_,
				sizeof(LayerAttrDefField) * fieldcount_);
	}
	if (fields_ == NULL) {
		fprintf(stderr, "Fail to alloc memory for fields.\n");
		return;
	}

	for (int i = 0; i < fieldcount_; ++i) {
		const LayerAttrDefField *field = attrdef.getField(i);
		fields_[i].sztitlelength_ = field->sztitlelength_;
		fields_[i].sztitle_ = (char *) malloc(
				fields_[i].sztitlelength_);
		if (fields_[i].sztitle_ == NULL) {
			fprintf(stderr, "Fail to alloc memory for field sztitle.\n");
			return;
		}

		memcpy(fields_[i].sztitle_, field->sztitle_, field->sztitlelength_);
		fields_[i].nWidth_ = field->nWidth_;
		fields_[i].nDecimals_ = field->nDecimals_;
		fields_[i].fieldtype_ = field->fieldtype_;
	}
	// set buffer flag.
	if (bufferflag_ == LATEST)
		bufferflag_ = STALE;
}

const char *LayerAttrDef::getBytes() {
	// alloc memory or return the buffered result.
	if (bufferflag_ == UNINITIALIZED) {
		buffer_ = (char *) malloc(attrdeflength_);
	} else if (bufferflag_ == STALE) {
		buffer_ = (char *) realloc(buffer_, attrdeflength_);
	} else {
		return buffer_;
	}

	char *bytes = buffer_;

	int offset = 0;
	// attrdeflength_
	memcpy(bytes + offset, &attrdeflength_, sizeof(attrdeflength_));
	offset += sizeof(attrdeflength_);

	memcpy(bytes + offset, &fieldcount_, sizeof(fieldcount_));
	offset += sizeof(fieldcount_);

	for (int ipoField = 0; ipoField < fieldcount_; ipoField++) {
		// sztitle
		memcpy(bytes + offset, &fields_[ipoField].sztitlelength_,
				sizeof(fields_[ipoField].sztitlelength_));

		memcpy(bytes + offset, fields_[ipoField].sztitle_,
				fields_[ipoField].sztitlelength_);

		offset += fields_[ipoField].sztitlelength_
				+ sizeof(fields_[ipoField].sztitlelength_);

		// nWidth
		memcpy(bytes + offset, &fields_[ipoField].nWidth_,
				sizeof(fields_[ipoField].nWidth_));
		offset += sizeof(fields_[ipoField].nWidth_);

		// nDecimals precision
		memcpy(bytes + offset, &fields_[ipoField].nDecimals_,
				sizeof(fields_[ipoField].nDecimals_));
		offset += sizeof(fields_[ipoField].nDecimals_);

		// fieldtype
		memcpy(bytes + offset, &fields_[ipoField].fieldtype_,
				sizeof(fields_[ipoField].fieldtype_));
		offset += sizeof(fields_[ipoField].fieldtype_);
	}

	assert(offset == attrdeflength_);

	bufferflag_ = LATEST;
	return buffer_;
}

int LayerAttrDef::getAttrDefLength() const {
	return attrdeflength_;
}

int LayerAttrDef::getFieldCount() const {
	return fieldcount_;
}

const LayerAttrDefField *LayerAttrDef::getFields() const {
	return fields_;
}

const LayerAttrDefField *LayerAttrDef::getField(int index) const {
	if (fields_ == NULL || index < 0 || index >= fieldcount_)
		return NULL;
	return &fields_[index];
}

/// @file layerAllRecords.cc
/// @author luliang@ict.ac.cn
/// @copybrief Copyright 2012 ICT, CAS. All rights reserved.
/// @version 0.6
/// @date 2013-06-25

#include "layerAllRecords.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <ogrsf_frmts.h>

LayerAllRecords::LayerAllRecords() :
  	recordlength_(0), recordcount_(0), fieldcount_(NULL), fields_(NULL), buffer_(
				NULL), bufferflag_(UNINITIALIZED) {
}

LayerAllRecords::LayerAllRecords(const LayerAllRecords & allrecords) :
		recordlength_(0), recordcount_(0), fieldcount_(NULL), fields_(NULL), buffer_(
				NULL), bufferflag_(UNINITIALIZED) {
	setAllRecords(allrecords);
}
LayerAllRecords::LayerAllRecords(const OGRLayer *layer) :
		recordlength_(0), recordcount_(0), fieldcount_(NULL), fields_(NULL), buffer_(
				NULL), bufferflag_(UNINITIALIZED) {
	setAllRecords(layer);
}

LayerAllRecords::LayerAllRecords(const char * bytes) :
		recordlength_(0), recordcount_(0), fieldcount_(NULL), fields_(NULL), buffer_(
				NULL), bufferflag_(UNINITIALIZED) {
	setAllRecords(bytes);
}

LayerAllRecords::~LayerAllRecords() {
	for (int i = 0; i < recordcount_; ++i) {
		for (int j = 0; j < fieldcount_; ++j) {
			int index = i * fieldcount_ + j;
			switch (fields_[index].fieldtype_) {
			case FTInteger:
				break;
			case FTReal:
				break;
			case FTString:
				char *str = fields_[index].field_.svalue_.str_;
				if (str)
					free(str);
				break;
			case FTBinary:
				char *bytes = fields_[index].field_.bvalue_.bytes_;
				if (bytes)
					free(bytes);
				break;
			case FTDate:
				break;
			default:
				break;
			}
		}
	}

	if (fields_)
		free(fields_);
	if (buffer_)
		free(buffer_);
}

void LayerAllRecords::setAllRecords(const OGRLayer *layer) {
	if (layer == NULL)
		return;
	recordlength_ = 0;
	// recordlength_
	recordlength_ += sizeof(recordlength_);
	// clear
	for (int i = 0; i < recordcount_; ++i) {
		for (int j = 0; j < fieldcount_; ++j) {
			int index = i * fieldcount_ + j;
			switch (fields_[index].fieldtype_) {
			case FTInteger:
				break;
			case FTReal:
				break;
			case FTString:
				char *str = fields_[index].field_.svalue_.str_;
				if (str)
					free(str);
				break;
			case FTBinary:
				char *bytes = fields_[index].field_.bvalue_.bytes_;
				if (bytes)
					free(bytes);
				break;
			case FTDate:
				break;
			default:
				break;
			}
		}
	}
	// recordcount_
	recordcount_ = 0;
	recordlength_ += sizeof(recordcount_);
	// fieldcount_
	fieldcount_ = layer->GetLayerDefn()->GetFieldCount();
	recordlength_ += sizeof(fieldcount_);

	layer->ResetReading();
	for (OGRFeature *feature = layer->GetNextFeature(); feature != NULL;
			feature = layer->GetNextFeature()) {
		OGRGeometry *geometry = feature->GetGeometryRef();
		if (geometry) {
			++recordcount_;

			for (int ifield = 0; ifield < fieldcount_; ++ifield) {
				OGRFieldDefn* poField = layer->GetLayerDefn()->GetFieldDefn(
						ifield);
				char fieldtype = (char) poField->GetType();
				recordlength_ += sizeof(fieldtype);
				switch (fieldtype) {
				case FTInteger:
					recordlength_ += sizeof(int);
					break;
				case FTReal:
					recordlength_ += sizeof(double);
					break;
				case FTString:
					const char *pstr = feature->GetFieldAsString(ifield);
					int strlength = strlen(pstr) + 1;
					recordlength_ += sizeof(strlength) + strlength;
					break;
				case FTBinary:
					int blobsize;
					feature->GetFieldAsBinary(ifield, &blobsize);
					int byteslength = blobsize;
					recordlength_ += sizeof(byteslength) + byteslength;
					break;
				case FTDate:
					recordlength_ += 7 * sizeof(int);
					break;
				default:
					break;
				}
			}
		}
		OGRFeature::DestroyFeature(feature);
	}

	if (fields_ == NULL) {
		fields_ = (LayerRecordField *) malloc(
				sizeof(LayerRecordField) * recordcount_ * fieldcount_);
	} else {
		fields_ = (LayerRecordField *) realloc(fields_,
				sizeof(LayerRecordField) * recordcount_ * fieldcount_);
	}
	if (fields_ == NULL) {
		fprintf(stderr, "Fail to alloc memory for record fields.\n");
		return;
	}

	layer->ResetReading();
	int ifeature = 0;
	for (OGRFeature *feature = layer->GetNextFeature(); feature != NULL;
			feature = layer->GetNextFeature()) {
		OGRGeometry *geometry = feature->GetGeometryRef();
		if (geometry) {

			for (int ifield = 0; ifield < fieldcount_; ++ifield) {
				OGRFieldDefn* poField = layer->GetLayerDefn()->GetFieldDefn(
						ifield);
				int index = ifeature * fieldcount_ * ifield;
				char fieldtype = (char) poField->GetType();
				fields_[index].fieldtype_ = fieldtype;

				switch (fieldtype) {
				case FTInteger:
					int ivalue = feature->GetFieldAsInteger(ifield);
					fields_[index].field_.ivalue_ = ivalue;
					break;
				case FTReal:
					double dvalue = feature->GetFieldAsDouble(ifield);
					fields_[index].field_.dvalue_ = dvalue;
					break;
				case FTString:
					const char *pstr = feature->GetFieldAsString(ifield);
					int strlength = strlen(pstr) + 1;
					fields_[index].field_.svalue_.strlength_ = strlength;
					fields_[index].field_.svalue_.str_ = (char *) malloc(
							strlength);
					if (fields_[index].field_.svalue_.str_ == NULL) {
						fprintf(
								stderr,
								"Fail to alloc memory for record field string.\n");
						return;
					}
					memcpy(fields_[index].field_.svalue_.str_, pstr, strlength);
					break;
				case FTBinary:
					int blobsize;
					unsigned char * bvalue = feature->GetFieldAsBinary(ifield,
							&blobsize);
					fields_[index].field_.bvalue_.byteslength_ = blobsize;
					fields_[index].field_.bvalue_.bytes_ = (char *) malloc(
							blobsize);
					if (fields_[index].field_.bvalue_.bytes_ == NULL) {
						fprintf(
								stderr,
								"Fail to alloc memory for record field bytes array.\n");
						return;
					}
					memcpy(fields_[index].field_.bvalue_.bytes_, bvalue,
							blobsize);
					break;
				case FTDate:
					int year, mon, day, hour, min, sec, tag;
					feature->GetFieldAsDateTime(ifield, &year, &mon, &day,
							&hour, &min, &sec, &tag);
					fields_[index].field_.tvalue_.year_ = year;
					fields_[index].field_.tvalue_.mon_ = mon;
					fields_[index].field_.tvalue_.day_ = day;
					fields_[index].field_.tvalue_.hour_ = hour;
					fields_[index].field_.tvalue_.min_ = min;
					fields_[index].field_.tvalue_.sec_ = sec;
					fields_[index].field_.tvalue_.tag_ = tag;
					break;
				default:
					break;
				}
			}
			++ifeature;
		}
		OGRFeature::DestroyFeature(feature);
	}

	// set buffer flag.
	if (bufferflag_ == LATEST)
		bufferflag_ = STALE;

}

void LayerAllRecords::setAllRecords(const char * bytes) {

}

void LayerAllRecords::setAllRecords(const LayerAllRecords & allrecords) {

}

const char *LayerAllRecords::getBytes() {
	// alloc memory or return the buffered result.
	if (bufferflag_ == UNINITIALIZED) {
		buffer_ = (char *) malloc(recordlength_);
	} else if (bufferflag_ == STALE) {
		buffer_ = (char *) realloc(buffer_, recordlength_);
	} else {
		return buffer_;
	}

	char *bytes = buffer_;

	int offset = 0;
	// recordlength_
	memcpy(bytes + offset, &recordlength_, sizeof(recordlength_));
	offset += sizeof(recordlength_);

	// recordcount_
	memcpy(bytes + offset, &recordcount_, sizeof(recordcount_));
	offset += sizeof(recordcount_);

	// fieldcount_
	memcpy(bytes + offset, &fieldcount_, sizeof(fieldcount_));
	offset += sizeof(fieldcount_);

	for (int i = 0; i < recordcount_; ++i) {
		for (int j = 0; j < fieldcount_; ++j) {
			int index = i * fieldcount_ + j;
			char fieldtype = fields_[index].fieldtype_;
			memcpy(bytes + offset, &fieldtype, sizeof(fieldtype));
			offset += sizeof(fieldtype);
			switch (fieldtype) {
			case FTInteger:
				int ivalue = fields_[index].field_.ivalue_;
				memcpy(bytes + offset, &ivalue, sizeof(ivalue));
				offset += sizeof(ivalue);
				break;
			case FTReal:
				double dvalue = fields_[index].field_.dvalue_;
				memcpy(bytes + offset, &dvalue, sizeof(dvalue));
				offset += sizeof(dvalue);
				break;
			case FTString:
				int strlength = fields_[index].field_.svalue_.strlength_;
				memcpy(bytes + offset, &strlength, sizeof(strlength));
				offset += sizeof(strlength);
				char *str = fields_[index].field_.svalue_.str_;
				memcpy(bytes + offset, str, strlength);
				break;
			case FTBinary:
				int byteslength = fields_[index].field_.bvalue_.byteslength_;
				memcpy(bytes + offset, &byteslength, sizeof(byteslength));
				offset += sizeof(byteslength);
				char *str = fields_[index].field_.bvalue_.bytes_;
				memcpy(bytes + offset, str, byteslength);
				break;
			case FTDate:
				FieldDateType & time = fields_[index].field_.tvalue_;
				memcpy(bytes + offset, &time.year_, sizeof(time.year_));
				offset += sizeof(time.year_);
				memcpy(bytes + offset, &time.mon_, sizeof(time.mon_));
				offset += sizeof(time.mon_);
				memcpy(bytes + offset, &time.day_, sizeof(time.day_));
				offset += sizeof(time.day_);
				memcpy(bytes + offset, &time.hour_, sizeof(time.hour_));
				offset += sizeof(time.hour_);
				memcpy(bytes + offset, &time.min_, sizeof(time.min_));
				offset += sizeof(time.min_);
				memcpy(bytes + offset, &time.sec_, sizeof(time.sec_));
				offset += sizeof(time.sec_);
				memcpy(bytes + offset, &time.tag_, sizeof(time.tag_));
				offset += sizeof(time.tag_);
				break;
			default:
				break;
			}
		}
	}

	assert(offset == recordlength_);

	bufferflag_ = LATEST;
	return buffer_;
}

int LayerAllRecords::getRecordLength() const {
	return recordlength_;
}

int LayerAllRecords::getRecordCount() const {
	return recordcount_;
}

int LayerAllRecords::getFieldCount() const {
	return fieldcount_;
}

const LayerRecordField *LayerAllRecords::getRecords() const {
	return fields_;
}

const LayerRecordField *LayerAllRecords::getRecord(int index) const {
	if (fields_ == NULL || index < 0 || index >= recordcount_)
		return NULL;
	return &fields_[index * fieldcount_];
}

const LayerRecordField *LayerAllRecords::getRecordField(int rindex,
		int findex) const {
	if (fields_ == NULL || rindex < 0 || rindex >= recordcount_ || findex < 0
			|| findex >= fieldcount_)
		return NULL;
	return &fields_[rindex * fieldcount_ + findex];
}

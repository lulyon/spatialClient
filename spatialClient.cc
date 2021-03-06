/// @file spatialClient.cc
/// @author luliang@ict.ac.cn
/// @copybrief Copyright 2013 ICT, CAS. All rights reserved.
/// @version 0.1
/// @date 2013-06-13

#include "spatialClient.h"

#include <string.h>
#include <assert.h>

#include <hiredis.h>
#include <ogrsf_frmts.h>

SpatialClient::SpatialClient() :
		con_(NULL) {
}

SpatialClient::~SpatialClient() {
	disconnect();
}

bool SpatialClient::connect(const char *ip, int port, int dbno) {
	con_ = redisConnect(ip, port);
	if (con_ == NULL || con_->err) {
		fprintf(stderr, "Connection error: %s\n", con_->errstr);
		if (con_)
			redisFree(con_);
		return false;
	}
	redisReply *reply = (redisReply *)redisCommand(con_, "select %d", dbno);
	if (reply == NULL || reply->type == REDIS_REPLY_ERROR) {
		fprintf(stderr, "Select db error: %s\n", reply->str);
		if (reply)
			freeReplyObject(reply);
		return false;
	}
	freeReplyObject(reply);
	return true;
}

void SpatialClient::disconnect() {
	redisContext * temp = con_;
	con_ = 0;
	if (temp) {
		redisFree(temp);
	}
}

char *SpatialClient::get(const char *key) const {
	return get(key, 0);
}

char *SpatialClient::get(const char *key, int *size) const {
	if (con_ == NULL) {
		fprintf(stderr, "Redis connection is not available.\n");
		return NULL;
	}
	redisReply *reply = (redisReply *)redisCommand(con_, "GET %s", key);
	if (reply == NULL || reply->type != REDIS_REPLY_STRING) {
		fprintf(stderr, "Redis reply error: not a string.\n");
		if (reply)
			freeReplyObject(reply);
		return NULL;
	}
	if (size)
		*size = reply->len;
	char *result = (char *) malloc((reply->len + 1) * sizeof(char));
	if (result == NULL) {
		fprintf(stderr, "redis get result malloc failed.\n");
		return NULL;
	}
	memcpy(result, reply->str, reply->len + 1);

	freeReplyObject(reply);
	return result;
}

bool SpatialClient::put(const char *key, const char *value) const {
	return put(key, value, 0);
}

bool SpatialClient::put(const char *key, const char *value, int size) const {
	if (con_ == NULL) {
		fprintf(stderr, "Redis connection is not available.\n");
		return false;
	}
	redisReply *reply = NULL;
	if (size) {
		reply = (redisReply *)redisCommand(con_, "SET %s %b", key, value, size);
	} else {
		reply = (redisReply *)redisCommand(con_, "SET %s %s", key, value);
	}

	if (reply == NULL || reply->type == REDIS_REPLY_ERROR) {
		fprintf(stderr, "Redis set command error: %s.\n", reply->str);
		if (reply)
			freeReplyObject(reply);
		return false;
	}

	freeReplyObject(reply);
	return true;
}

void SpatialClient::putLayer(const char *key, OGRLayer *layer) const {
	if (key == NULL) {
		fprintf(stderr, "Empty key.\n");
		return;
	}
	if (layer == NULL) {
		fprintf(stderr, "Empty OGRLayer.\n");
		return;
	}
	char *bytes = serialize(layer);
	if (bytes == NULL) {
		fprintf(stderr, "Nil OGRLayer bytes.\n");
		return;
	}
	int length = 0;
	memcpy(&length, bytes, sizeof(length));
	put(key, bytes, length);
}

OGRLayer *SpatialClient::getLayer(const char *key) const {
	if (key == NULL) {
		fprintf(stderr, "Empty key.\n");
		return NULL;
	}
	char *bytes = get(key);
	if (bytes == NULL) {
		fprintf(stderr, "Fail to get the layer bytes.\n");
		return NULL;
	}
	OGRLayer *layer = deserialize(bytes);
	return layer;
}

char *SpatialClient::serialize(OGRLayer *poLayer) const {
	if (poLayer == NULL)
		return NULL;

	int length = 0;
	int metadatalength = 0;
	int attributedeflength = 0;
	int featurelength = 0;
	int attributerecordlength = 0;

	// compute metadata size
	const char *layername = poLayer->GetName();
	int layernamelength = strlen(layername) + 1;
	metadatalength += layernamelength + sizeof(layernamelength);

	int geotype = (int) poLayer->GetGeomType();
	metadatalength += sizeof(geotype);

	char *strWKT = NULL;
	OGRSpatialReference *poSR = poLayer->GetSpatialRef();
	if (poSR) {
		poSR->exportToWkt(&strWKT);
	} else {
		fprintf(stdout, "since no srs specified,default would be assigned.");
		strWKT = "";
	}
	int strWKTlength = strlen(strWKT) + 1;
	metadatalength += strWKTlength + sizeof(strWKTlength);
	length += metadatalength + sizeof(metadatalength);

	//compute attribute definition data size
	int fieldcount = poLayer->GetLayerDefn()->GetFieldCount();
	attributedeflength += sizeof(fieldcount);

	for (int ipoField = 0; ipoField < fieldcount; ipoField++) {
		OGRFieldDefn* poField = poLayer->GetLayerDefn()->GetFieldDefn(ipoField);
		const char *sztitle = poField->GetNameRef();
		int sztitlelength = strlen(sztitle) + 1;
		attributedeflength += sztitlelength + sizeof(sztitlelength);

		int nWidth = poField->GetWidth();
		attributedeflength += sizeof(nWidth);

		int nDecimals = poField->GetPrecision();
		attributedeflength += sizeof(nDecimals);

		char fieldtype = (char) poField->GetType();
		attributedeflength += sizeof(fieldtype);
	}
	length += attributedeflength + sizeof(attributedeflength);

	// compute feature size and attribute record size.
	int featurecount = 0;
	featurelength += sizeof(featurecount);

	int attributerecordcount = 0;
	attributerecordlength += sizeof(attributerecordcount);
	attributerecordlength += sizeof(fieldcount);
	poLayer->ResetReading();
	for (OGRFeature *feature = poLayer->GetNextFeature(); feature != NULL;
			feature = poLayer->GetNextFeature()) {
		OGRGeometry *geometry = feature->GetGeometryRef();
		if (geometry) {
			// int geometrytype = (int)geometry->getGeometryType();
			featurelength += sizeof(int);
			int wkbsize = geometry->WkbSize();
			featurelength += sizeof(wkbsize) + wkbsize;

			++featurecount;
			++attributerecordcount;

			for (int ifield = 0; ifield < fieldcount; ++ifield) {
				OGRFieldDefn* poField = poLayer->GetLayerDefn()->GetFieldDefn(
						ifield);
				char attributetype = (char) poField->GetType();
				attributerecordlength += sizeof(attributetype);
				switch (attributetype) {
				case OFTInteger:
					attributerecordlength += sizeof(int);
					break;
				case OFTReal:
					attributerecordlength += sizeof(double);
					break;
				case OFTString: {
					const char *pstr = feature->GetFieldAsString(ifield);
					int strlength = strlen(pstr) + 1;
					attributerecordlength += sizeof(strlength) + strlength;
					break;
				}
				case OFTBinary: {
					int blobsize;
					feature->GetFieldAsBinary(ifield, &blobsize);
					int byteslength = blobsize;
					attributerecordlength += sizeof(byteslength) + byteslength;
					break;
				}
				case OFTDate:
					attributerecordlength += 7 * sizeof(int); //int year, mon, day, hour, min, sec, tag;
					break;
				default:
					continue;
				}
			}
		}
		OGRFeature::DestroyFeature(feature);
	}
	length += featurelength + sizeof(featurelength);
	length += attributerecordlength + sizeof(attributerecordlength);
	length += sizeof(length);

	// alloc memory for serialization.
	char *bytes = (char *) malloc((length + 10));
	if (bytes == NULL) {
		fprintf(stderr, "Fail to alloc memory for bytes.\n");
		return NULL;
	}

	assert(bytes != NULL);

	int offset = 0;
	memcpy(bytes + offset, &length, sizeof(length));
	offset += sizeof(length);
	// serialize metadata.
	// metadatalength
	memcpy(bytes + offset, &metadatalength, sizeof(metadatalength));
	offset += sizeof(metadatalength);

	// layername
	memcpy(bytes + offset, &layernamelength, sizeof(layernamelength));
	offset += sizeof(layernamelength);
	memcpy(bytes + offset, layername, layernamelength);
	offset += layernamelength;

	// geometry type
	memcpy(bytes + offset, &geotype, sizeof(geotype));
	offset += sizeof(geotype);

	// strWKT geo reference
	memcpy(bytes + offset, &strWKTlength, sizeof(strWKTlength));
	offset += sizeof(strWKTlength);
	memcpy(bytes + offset, strWKT, strWKTlength);
	offset += strWKTlength;

	//serialize attribute definition data
	memcpy(bytes + offset, &attributedeflength, sizeof(attributedeflength));
	offset += sizeof(attributedeflength);
	memcpy(bytes + offset, &fieldcount, sizeof(fieldcount));
	offset += sizeof(fieldcount);
	for (int ipoField = 0; ipoField < fieldcount; ipoField++) {
		OGRFieldDefn* poField = poLayer->GetLayerDefn()->GetFieldDefn(ipoField);
		const char *sztitle = poField->GetNameRef();
		int sztitlelength = strlen(sztitle) + 1;

		memcpy(bytes + offset, &sztitlelength, sizeof(sztitlelength));
		offset += sizeof(sztitlelength);
		memcpy(bytes + offset, sztitle, sztitlelength);
		offset += sztitlelength;

		int nWidth = poField->GetWidth();
		memcpy(bytes + offset, &nWidth, sizeof(nWidth));
		offset += sizeof(nWidth);

		int nDecimals = poField->GetPrecision();
		memcpy(bytes + offset, &nDecimals, sizeof(nDecimals));
		offset += sizeof(nDecimals);

		char fieldtype = (char) poField->GetType();
		memcpy(bytes + offset, &fieldtype, sizeof(fieldtype));
		offset += sizeof(fieldtype);
	}

	// serialize feature size and attribute record.
	memcpy(bytes + offset, &featurelength, sizeof(featurelength));
	offset += sizeof(featurelength);
	memcpy(bytes + offset, &featurecount, sizeof(featurecount));
	offset += sizeof(featurecount);

	int offset2 = offset + featurelength;
	memcpy(bytes + offset2, &attributerecordlength,
			sizeof(attributerecordlength));
	offset2 += sizeof(attributerecordlength);

	memcpy(bytes + offset2, &attributerecordcount,
			sizeof(attributerecordlength));
	offset2 += sizeof(attributerecordcount);

	memcpy(bytes + offset2, &fieldcount, sizeof(fieldcount));
	offset2 += sizeof(fieldcount);

	poLayer->ResetReading();
	for (OGRFeature *feature = poLayer->GetNextFeature(); feature != NULL;
			feature = poLayer->GetNextFeature()) {
		OGRGeometry *geometry = feature->GetGeometryRef();
		if (geometry) {
			int geometrytype = (int) geometry->getGeometryType();
			memcpy(bytes + offset, &geometrytype, sizeof(geometrytype));
			offset += sizeof(geometrytype);

			int wkbsize = geometry->WkbSize();
			memcpy(bytes + offset, &wkbsize, sizeof(wkbsize));
			offset += sizeof(wkbsize);

			geometry->exportToWkb((OGRwkbByteOrder) wkbNDR,
					(unsigned char *) (bytes + offset));
			offset += geometry->WkbSize();

			for (int ifield = 0; ifield < fieldcount; ++ifield) {
				OGRFieldDefn* poField = poLayer->GetLayerDefn()->GetFieldDefn(
						ifield);
				char attributetype = (char) poField->GetType();
				memcpy(bytes + offset2, &attributetype, sizeof(attributetype));
				offset2 += sizeof(attributetype);

				switch (attributetype) {
				case OFTInteger: {
					int ivalue = feature->GetFieldAsInteger(ifield);
					memcpy(bytes + offset2, &ivalue, sizeof(ivalue));
					offset2 += sizeof(ivalue);
					break;
				}
				case OFTReal: {
					double dvalue = feature->GetFieldAsDouble(ifield);
					memcpy(bytes + offset2, &dvalue, sizeof(dvalue));
					offset2 += sizeof(dvalue);
					break;
				}
				case OFTString: {
					const char *pstr = feature->GetFieldAsString(ifield);
					int strlength = strlen(pstr) + 1;
					memcpy(bytes + offset2, &strlength, sizeof(strlength));
					offset2 += sizeof(strlength);
					memcpy(bytes + offset2, pstr, strlength);
					offset2 += strlength;
					break;
				}
				case OFTBinary: {
					int blobsize;
					unsigned char * bvalue = feature->GetFieldAsBinary(ifield,
							&blobsize);
					int bvaluelength = blobsize;
					memcpy(bytes + offset2, &bvaluelength,
							sizeof(bvaluelength));
					offset2 += sizeof(bvaluelength);
					memcpy(bytes + offset2, bvalue, bvaluelength);
					offset2 += bvaluelength;
					break;
				}
				case OFTDate: {
					int year, mon, day, hour, min, sec, tag;
					feature->GetFieldAsDateTime(ifield, &year, &mon, &day,
							&hour, &min, &sec, &tag);
					memcpy(bytes + offset2, &year, sizeof(year));
					offset2 += sizeof(year);
					memcpy(bytes + offset2, &mon, sizeof(mon));
					offset2 += sizeof(mon);
					memcpy(bytes + offset2, &day, sizeof(day));
					offset2 += sizeof(day);
					memcpy(bytes + offset2, &hour, sizeof(hour));
					offset2 += sizeof(hour);
					memcpy(bytes + offset2, &min, sizeof(min));
					offset2 += sizeof(min);
					memcpy(bytes + offset2, &sec, sizeof(sec));
					offset2 += sizeof(sec);
					memcpy(bytes + offset2, &tag, sizeof(tag));
					offset2 += sizeof(tag);
					break;
				}
				default:
					break;
				}
			}
		}
		OGRFeature::DestroyFeature(feature);
	}

	assert(offset2 == length);

	return bytes;
}

OGRLayer *SpatialClient::deserialize(const char *bytes) const {
	OGRRegisterAll();
	OGRSFDriver *pdriver =
			OGRSFDriverRegistrar::GetRegistrar()->GetDriverByName("Memory");
	if (!pdriver)
		return NULL;

	OGRDataSource *pds = pdriver->CreateDataSource("DRAMA");
	if (!pds)
		return NULL;

	int offset = 0;
	int length = 0;
	memcpy(&length, bytes + offset, sizeof(length));
	offset += sizeof(length);
	// serialize metadata.
	// metadatalength
	int metadatalength = 0;
	memcpy(&metadatalength, bytes + offset, sizeof(metadatalength));
	offset += sizeof(metadatalength);

	// layername
	int layernamelength = 0;
	memcpy(&layernamelength, bytes + offset, sizeof(layernamelength));
	offset += sizeof(layernamelength);

	char *layername = (char *) malloc(layernamelength);
	if (layername == NULL) {
		fprintf(stderr, "Fail to alloc memory for layername.\n");
		return NULL;
	}
	memcpy(layername, bytes + offset, layernamelength);
	offset += layernamelength;

	// geometry type
	int geotype = 0;
	memcpy(&geotype, bytes + offset, sizeof(geotype));
	offset += sizeof(geotype);

	// strWKT georeference
	int strWKTlength = 0;
	memcpy(&strWKTlength, bytes + offset, sizeof(strWKTlength));
	offset += sizeof(strWKTlength);
	char *strWKT = (char *)malloc(strWKTlength);
	if (strWKT == NULL) {
		fprintf(stderr, "Fail to alloc memory for strWKT.\n");
		return NULL;
	}
	memcpy(strWKT, bytes + offset, strWKTlength);
	offset += strWKTlength;

	OGRSpatialReference srs;
	if (strcmp(strWKT, "") == 0) {
		fprintf(stdout, "since no srs specified, EPSG:4326 would be assigned.");
		srs.SetWellKnownGeogCS("EPSG:4326");
	} else {
		srs.importFromWkt(&strWKT);
	}

	char **papszOptions = NULL;
	papszOptions = CSLSetNameValue(papszOptions, "OVERWRITE", "YES");
	OGRLayer *poLayer = pds->CreateLayer(layername, &srs,
			(OGRwkbGeometryType) geotype, papszOptions);

	assert(poLayer);
	CSLDestroy(papszOptions);
	free(layername);

	//serialize attribute definition data
	// attributedeflength
	int attributedeflength = 0;
	memcpy(&attributedeflength, bytes + offset, sizeof(attributedeflength));
	offset += sizeof(attributedeflength);

	// fieldcount
	int fieldcount = 0;
	memcpy(&fieldcount, bytes + offset, sizeof(fieldcount));
	offset += sizeof(fieldcount);

	for (int ipoField = 0; ipoField < fieldcount; ipoField++) {
		// sztitle
		int sztitlelength = 0;
		memcpy(&sztitlelength, bytes + offset, sizeof(sztitlelength));
		offset += sizeof(sztitlelength);
		char *sztitle = (char *) malloc(sztitlelength);
		if (sztitle == NULL) {
			fprintf(stderr, "Fail to alloc memory for sztitle.\n");
			return NULL;
		}
		memcpy(sztitle, bytes + offset, sztitlelength);
		offset += sztitlelength;

		// nWidth
		int nWidth = 0;
		memcpy(&nWidth, bytes + offset, sizeof(nWidth));
		offset += sizeof(nWidth);

		// nDecimals precision
		int nDecimals = 0;
		memcpy(&nDecimals, bytes + offset, sizeof(nDecimals));
		offset += sizeof(nDecimals);

		// fieldtype
		char fieldtype = 0;
		memcpy(&fieldtype, bytes + offset, sizeof(fieldtype));
		offset += sizeof(fieldtype);

		OGRFieldDefn oField(sztitle, (OGRFieldType)fieldtype);
		oField.SetWidth(nWidth);
		oField.SetPrecision(nDecimals);

		poLayer->CreateField(&oField);
		free(sztitle);
	}

	// deserialize feature size and attribute record.
	// featurelength
	int featurelength = 0;
	memcpy(&featurelength, bytes + offset, sizeof(featurelength));
	offset += sizeof(featurelength);

	// featurecount
	int featurecount = 0;
	memcpy(&featurecount, bytes + offset, sizeof(featurecount));
	offset += sizeof(featurecount);

	int offset2 = offset + featurelength;
	int attributerecordlength = 0;
	memcpy(&attributerecordlength, bytes + offset2,
			sizeof(attributerecordlength));
	offset2 += sizeof(attributerecordlength);

	int attributerecordcount = 0;
	memcpy(&attributerecordcount, bytes + offset2,
			sizeof(attributerecordcount));
	offset2 += sizeof(attributerecordcount);

	int recordfieldcount = 0;
	memcpy(&recordfieldcount, bytes + offset2, sizeof(recordfieldcount));
	offset2 += sizeof(recordfieldcount);

	OGRFeatureDefn *defn = poLayer->GetLayerDefn();
	for (int iFeature = 0; iFeature < featurecount; iFeature++) {
		int geointtype = 0;
		memcpy(&geointtype, bytes + offset, sizeof(geointtype));
		offset += sizeof(geointtype);
		OGRwkbGeometryType geometrytype = (OGRwkbGeometryType) geointtype;
		OGRGeometry *geometry = NULL;
		switch (geometrytype) {
		case wkbPoint:
		case wkbPoint25D:
			geometry = new OGRPoint();
			break;
		case wkbLineString:
		case wkbLineString25D:
			geometry = new OGRLineString();
			break;
		case wkbPolygon:
		case wkbPolygon25D:
			geometry = new OGRPolygon();
			break;
		case wkbMultiPoint:
		case wkbMultiPoint25D:
			geometry = new OGRMultiPoint();
			break;
		case wkbMultiLineString:
		case wkbMultiLineString25D:
			geometry = new OGRMultiLineString();
			break;
		case wkbMultiPolygon:
		case wkbMultiPolygon25D:
			geometry = new OGRMultiPolygon();
			break;
		case wkbGeometryCollection:
		case wkbGeometryCollection25D:
			geometry = new OGRGeometryCollection();
			break;
		default:
			break;
		}
		if (geometry) {
			// wkb feature
			int wkbsize = 0;
			memcpy(&wkbsize, bytes + offset, sizeof(wkbsize));
			offset += sizeof(wkbsize);
			geometry->importFromWkb((unsigned char *) (bytes + offset),
					wkbsize);
			offset += geometry->WkbSize();

			OGRFeature *feature = new OGRFeature(defn);
			feature->SetGeometry(geometry);
			poLayer->CreateFeature(feature);

			for (int ifield = 0; ifield < recordfieldcount; ++ifield) {
				char ftype = 0;
				memcpy(&ftype, bytes + offset2, sizeof(ftype));
				offset2 += sizeof(ftype);
				OGRFieldType fieldtype = (OGRFieldType) ftype;
				switch (fieldtype) {
				case OFTInteger: {
					int ivalue = 0;
					memcpy(&ivalue, bytes + offset2, sizeof(ivalue));
					offset2 += sizeof(ivalue);

					feature->SetField(ifield, ivalue);
					break;
				}
				case OFTReal: {
					double dvalue = 0;
					memcpy(&dvalue, bytes + offset2, sizeof(dvalue));
					offset2 += sizeof(dvalue);

					feature->SetField(ifield, dvalue);
					break;
				}
				case OFTString: {
					int strlength;
					memcpy(&strlength, bytes + offset2, sizeof(strlength));
					offset2 += sizeof(strlength);
					char *pstr = (char *) malloc(strlength);
					if (pstr == NULL) {
						fprintf(stderr, "Fail to alloc memory for pstr.\n");
						return NULL;
					}
					memcpy(pstr, bytes + offset2, strlength);
					offset2 += strlength;

					feature->SetField(ifield, pstr);
					break;
				}
				case OFTBinary: {
					int bvaluelength = 0;
					memcpy(&bvaluelength, bytes + offset2,
							sizeof(bvaluelength));
					offset2 += sizeof(bvaluelength);
					char *bvalue = (char *) malloc(bvaluelength);
					if (bvalue == NULL) {
						fprintf(stderr, "Fail to alloc memory for bvalue.\n");
						return NULL;
					}

					memcpy(bvalue, bytes + offset2, bvaluelength);
					offset2 += bvaluelength;

					feature->SetField(ifield, bvaluelength,
							(unsigned char *) bvalue);
					break;
				}
				case OFTDate: {
					int year, mon, day, hour, min, sec, tag;
					year = mon = day = hour = min = sec = tag;

					memcpy(&year, bytes + offset2, sizeof(year));
					offset2 += sizeof(year);
					memcpy(&mon, bytes + offset2, sizeof(mon));
					offset2 += sizeof(mon);
					memcpy(&day, bytes + offset2, sizeof(day));
					offset2 += sizeof(day);
					memcpy(&hour, bytes + offset2, sizeof(hour));
					offset2 += sizeof(hour);
					memcpy(&min, bytes + offset2, sizeof(min));
					offset2 += sizeof(min);
					memcpy(&sec, bytes + offset2, sizeof(sec));
					offset2 += sizeof(sec);
					memcpy(&tag, bytes + offset2, sizeof(tag));
					offset2 += sizeof(tag);

					feature->SetField(ifield, year, mon, day, hour, min, sec,
							tag);
					break;
				}
				default:
					break;
				}

			}

		}
	}

	assert(offset2 == length);

	return poLayer;
}

void SpatialClient::putMetadata(const char *key, OGRLayer *layer) const {
	LayerMetadata metadata(layer);
	putMetadata(key, &metadata);
}

void SpatialClient::putMetadata(const char *key,
		LayerMetadata *metadata) const {
	if (key == NULL) {
		fprintf(stderr, "Empty key.\n");
		return;
	}

	if (metadata == NULL) {
		fprintf(stderr, "Nil LayerMetadata object.\n");
		return;
	}

	const char *bytes = metadata->getBytes();
	if (bytes == NULL) {
		fprintf(stderr, "Fail to get the metadata bytes.\n");
		return;
	}
	int metadatalength = metadata->getMetadataLength();

	put(key, bytes, metadatalength);
}

LayerMetadata * SpatialClient::getMetadata(const char *key) const {
	if (key == NULL) {
		fprintf(stderr, "Empty key.\n");
		return NULL;
	}
	char *bytes = get(key);

	if (bytes == NULL) {
		fprintf(stderr, "Fail to get the metadata bytes.\n");
		return NULL;
	}
	LayerMetadata *metadata = new LayerMetadata(bytes);
	return metadata;
}

void SpatialClient::putAttributeDef(const char *key,
		OGRLayer *layer) const {
	LayerAttrDef attrdef(layer);
	putAttributeDef(key, &attrdef);
}
void SpatialClient::putAttributeDef(const char *key,
		LayerAttrDef * attrdef) const {
	if (key == NULL) {
		fprintf(stderr, "Empty key.\n");
		return;
	}

	if (attrdef == NULL) {
		fprintf(stderr, "Nil LayerAttrDef object.\n");
		return;
	}

	const char *bytes = attrdef->getBytes();
	if (bytes == NULL) {
		fprintf(stderr, "Fail to get the attribute definition bytes.\n");
		return;
	}
	int attrdeflength = attrdef->getAttrDefLength();

	put(key, bytes, attrdeflength);

}
LayerAttrDef * SpatialClient::getAttributeDef(const char *key) const {
	if (key == NULL) {
		fprintf(stderr, "Empty key.\n");
		return NULL;
	}
	char *bytes = get(key);

	if (bytes == NULL) {
		fprintf(stderr, "Fail to get the attribute definition bytes.\n");
		return NULL;
	}
	LayerAttrDef *attrdef = new LayerAttrDef(bytes);
	return attrdef;
}

void SpatialClient::putAllFeatures(const char *key,
		OGRLayer *layer) const {
	LayerAllFeatures features(layer);
	putAllFeatures(key, &features);
}

void SpatialClient::putAllFeatures(const char *key,
		LayerAllFeatures * allfeatures) const {
	if (key == NULL) {
		fprintf(stderr, "Empty key.\n");
		return;
	}

	if (allfeatures == NULL) {
		fprintf(stderr, "Nil AllFeatures object.\n");
		return;
	}

	const char *bytes = allfeatures->getBytes();
	if (bytes == NULL) {
		fprintf(stderr, "Fail to get the attribute definition bytes.\n");
		return;
	}
	int featurelength = allfeatures->getFeatureLength();

	put(key, bytes, featurelength);
}

LayerAllFeatures * SpatialClient::getAllFeatures(const char *key) const {
	if (key == NULL) {
		fprintf(stderr, "Empty key.\n");
		return NULL;
	}
	char *bytes = get(key);

	if (bytes == NULL) {
		fprintf(stderr, "Fail to get the layer features bytes.\n");
		return NULL;
	}
	LayerAllFeatures *features = new LayerAllFeatures(bytes);
	return features;
}

void SpatialClient::putAllRecords(const char *key,
		OGRLayer *layer) const {
	LayerAllRecords records(layer);
	putAllRecords(key, &records);
}

void SpatialClient::putAllRecords(const char *key,
		LayerAllRecords * allrecords) const {
	if (key == NULL) {
		fprintf(stderr, "Empty key.\n");
		return;
	}

	if (allrecords == NULL) {
		fprintf(stderr, "Nil AllRecords object.\n");
		return;
	}

	const char *bytes = allrecords->getBytes();
	if (bytes == NULL) {
		fprintf(stderr, "Fail to get the attribute definition bytes.\n");
		return;
	}
	int recordlength = allrecords->getRecordLength();

	put(key, bytes, recordlength);
}

LayerAllRecords * SpatialClient::getAllRecords(const char *key) const {
	if (key == NULL) {
		fprintf(stderr, "Empty key.\n");
		return NULL;
	}
	char *bytes = get(key);

	if (bytes == NULL) {
		fprintf(stderr, "Fail to get the layer features bytes.\n");
		return NULL;
	}
	LayerAllRecords *records = new LayerAllRecords(bytes);
	return records;
}

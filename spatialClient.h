/// @file spatialClient.h
/// @author luliang@ict.ac.cn
/// @copybrief Copyright 2012 ICT, CAS. All rights reserved.
/// @version 0.6
/// @date 2013-06-13

#ifndef SPATIALCLIENT_H_
#define SPATIALCLIENT_H_

#include "layerMetadata.h"

struct redisContext;
class OGRLayer;
class LayerMetadata;

class SpatialClient {
public:
	SpatialClient();
	~SpatialClient();

	bool connect(const char *ip = "127.0.0.1", int port = 6379, int dbno = 0);
	void disconnect();

	char *get(const char *key) const;
	char *get(const char *key, int *size) const; // size: return size of the value.
	bool put(const char *key, const char *value) const;
	bool put(const char *key, const char *value, int size) const; //size means value size.

	void putLayer(const char *key, const OGRLayer *layer) const;
	OGRLayer *getLayer(const char *key) const;

	void putMetadata(const char *key, const OGRLayer *layer) const;
	void putMetadata(const char *key, LayerMetadata *metadata) const;
	LayerMetadata * getMetadata(const char *key) const;

	void putAttributeDef(const char *key, const OGRLayer *layer) const;
	void putAttributeDef(const char *key, LayerAttrDef * attrdef) const;
	LayerAttrDef * getAttributeDef(const char *key) const;

	void getFeatureCount();
	void putFeature();
	void getFeature();

	void getAttributeCount();
	void putAttributeRecord();
	void getAttributeRecord();
private:
	SpatialClient(const SpatialClient &);
	void operator=(const SpatialClient &);
	char *serialize(const OGRLayer *poLayer) const;
	OGRLayer *deserialize(const char *bytes) const;
	redisContext *con_;
};

#endif /* SPATIALCLIENT_H_ */

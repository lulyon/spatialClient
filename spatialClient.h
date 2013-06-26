/// @file spatialClient.h
/// @author luliang@ict.ac.cn
/// @copybrief Copyright 2013 ICT, CAS. All rights reserved.
/// @version 0.1
/// @date 2013-06-13

#ifndef SPATIALCLIENT_H_
#define SPATIALCLIENT_H_

#include "layerMetadata.h"
#include "layerAttrDef.h"
#include "layerAllFeatures.h"
#include "layerAllRecords.h"

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

	void putLayer(const char *key, OGRLayer *layer) const;
	OGRLayer *getLayer(const char *key) const;

	void putMetadata(const char *key, OGRLayer *layer) const;
	void putMetadata(const char *key, LayerMetadata *metadata) const;
	LayerMetadata * getMetadata(const char *key) const;

	void putAttributeDef(const char *key, OGRLayer *layer) const;
	void putAttributeDef(const char *key, LayerAttrDef * attrdef) const;
	LayerAttrDef * getAttributeDef(const char *key) const;

	void putAllFeatures(const char *key, OGRLayer *layer) const;
	void putAllFeatures(const char *key, LayerAllFeatures * allfeatures) const;
	LayerAllFeatures * getAllFeatures(const char *key) const;

	void putAllRecords(const char *key, OGRLayer *layer) const;
	void putAllRecords(const char *key, LayerAllRecords * allrecords) const;
	LayerAllRecords * getAllRecords(const char *key) const;
private:
	SpatialClient(const SpatialClient &);
	void operator=(const SpatialClient &);
	char *serialize(OGRLayer *poLayer) const;
	OGRLayer *deserialize(const char *bytes) const;
	redisContext *con_;
};

#endif /* SPATIALCLIENT_H_ */

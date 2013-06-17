/// @file spatialClient.h
/// @author luliang@ict.ac.cn
/// @copybrief Copyright 2012 ICT, CAS. All rights reserved.
/// @version 0.6
/// @date 2013-06-13

#ifndef SPATIALCLIENT_H_
#define SPATIALCLIENT_H_

struct redisContext;
class OGRLayer;

class SpatialClient {
public:
  SpatialClient();
	~SpatialClient();

	bool connect(const char *ip = "127.0.0.1", int port = 6379, int dbno = 0);
	void disconnect();

	char *get(const char *key);
	char *get(const char *key, int *size); // size: return size of the value.
	bool put(const char *key, const char *value);
	bool put(const char *key, const char *value, int size); //size means value size.

	void putLayer(const OGRLayer *layer);
	OGRLayer *getLayer(const char *key);

	void putMetadata();
	void getMetadata();

	void getFeatureCount();
	void putFeature();
	void getFeature();
	void putAttributeDef();
	void getAttributeDef();

	void getAttributeCount();
	void putAttributeRecord();
	void getAttributeRecord();
private:
	char *serialize(const OGRLayer *layer);
	OGRLayer *deserialize(const char *str);
	redisContext *con_;
};

#endif /* SPATIALCLIENT_H_ */

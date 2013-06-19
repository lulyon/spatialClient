/// @file layerMetadata.h
/// @author luliang@ict.ac.cn
/// @copybrief Copyright 2012 ICT, CAS. All rights reserved.
/// @version 0.6
/// @date 2013-06-19

#ifndef LAYERMETADATA_H_
#define LAYERMETADATA_H_

class OGRLayer;

class LayerMetadata
{
public:

	LayerMetadata();
	LayerMetadata(const OGRLayer *layer);
	LayerMetadata(const char * bytes);
	~LayerMetadata();

	const char *getBytes();

	int getMetadataLength() const;

	int getLayernameLength() const;
	const char *getLayername() const;

	int getGeotype() const;

	int getStrWKTlength() const;
	const char *getStrWKT() const;

	void setMetadata(const OGRLayer *layer);
	void setMetadata(const char * bytes);

private:
	typedef enum {UNINITIALIZED, STALE, LATEST} BufferFlagType;

	int metadatalength_;
	int layernamelength_;
	char * layername_;
	int geotype_;
	int strWKTlength_;
	char * strWKT_;

	char *buffer_;
	BufferFlagType bufferflag_;
};


#endif /* LAYERMETADATA_H_ */

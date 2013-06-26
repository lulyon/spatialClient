/// @file layerAllFeatures.h
/// @author luliang@ict.ac.cn
/// @copybrief Copyright 2012 ICT, CAS. All rights reserved.
/// @version 0.6
/// @date 2013-06-25

#ifndef LAYERALLFEATURES_H_
#define LAYERALLFEATURES_H_

class OGRLayer;

typedef struct {
	int geometrytype_;
	int wkbsize_;
	char *wkbbytes_;
} LayerFeature;

class LayerAllFeatures {
public:
	LayerAllFeatures();
	LayerAllFeatures(const LayerAllFeatures & allfeatures);
	LayerAllFeatures(const OGRLayer *layer);
	LayerAllFeatures(const char * bytes);
	~LayerAllFeatures();

	const char *getBytes();

	int getFeatureLength() const;
	int getFeatureCount() const;
	const LayerFeature *getFeatures() const;
	const LayerFeature *getFeature(int index) const;

	void setAllFeatures(const OGRLayer *layer);
	void setAllFeatures(const char * bytes);
	void setAllFeatures(const LayerAllFeatures & allfeatures);

private:
	typedef enum {
		UNINITIALIZED, STALE, LATEST
	} BufferFlagType;

	void operator=(const LayerAllRecords &);

	int featurelength_;
	int featurecount_;

	LayerFeature *features_;

	char *buffer_;
	BufferFlagType bufferflag_;
};

#endif /* LAYERALLFEATURES_H_ */

/*
 * layerAttrDef.h
 *
 *  Created on: 2013-6-21
 *      Author: luliang
 */

#ifndef LAYERATTRDEF_H_
#define LAYERATTRDEF_H_

class OGRLayer;

typedef struct {
  int sztitlelength_;
	char *sztitle_;

	int nWidth_;
	int nDecimals_;
	char fieldtype_;
} LayerAttrDefField;

class LayerAttrDef {
public:
	LayerAttrDef();
	LayerAttrDef(const LayerAttrDef & attrdef);
	LayerAttrDef(const OGRLayer *layer);
	LayerAttrDef(const char * bytes);
	~LayerAttrDef();

	const char *getBytes();

	int getAttrDefLength() const;
	int getFieldCount() const;
	const LayerAttrDefField *getFields() const;
	const LayerAttrDefField *getField(int index) const;

	void setAttrDef(const OGRLayer *layer);
	void setAttrDef(const char * bytes);
	void setAttrDef(const LayerAttrDef & attrdef);
private:
	typedef enum {
		UNINITIALIZED, STALE, LATEST
	} BufferFlagType;

	void operator=(const LayerAttrDef &);

	int attrdeflength_;
	int fieldcount_;
	LayerAttrDefField *fields_;

	char *buffer_;
	BufferFlagType bufferflag_;
};

#endif /* LAYERATTRDEF_H_ */

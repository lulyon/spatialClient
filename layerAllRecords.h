/// @file layerAllRecords.h
/// @author luliang@ict.ac.cn
/// @copybrief Copyright 2012 ICT, CAS. All rights reserved.
/// @version 0.6
/// @date 2013-06-25

#ifndef LAYERALLRECORDS_H_
#define LAYERALLRECORDS_H_

typedef enum {
  FTInteger = 0, FTReal = 2, FTString = 4, FTBinary = 8, FTDate = 9
} FieldType;

typedef struct {
	int strlength_;
	char * str_;
} FieldStringType;

typedef struct {
	int byteslength_;
	char * bytes_;
} FieldBinaryType;

typedef struct {
	int year_, mon_, day_, hour_, min_, sec_, tag_;
} FieldDateType;

typedef struct {
	char fieldtype_;
	union {
		int ivalue_;
		double dvalue_;
		FieldStringType svalue_;
		FieldBinaryType bvalue_;
		FieldDateType tvalue_;
	} field_;

} LayerRecordField;

class LayerAllRecords {
public:
	LayerAllRecords();
	LayerAllRecords(const LayerAllRecords & allrecords);
	LayerAllRecords(const OGRLayer *layer);
	LayerAllRecords(const char * bytes);
	~LayerAllRecords();

	const char *getBytes();

	int getRecordLength() const;
	int getRecordCount() const;
	int getFieldCount() const;
	const LayerRecordField *getRecords() const;
	const LayerRecordField *getRecord(int index) const;
	const LayerRecordField *getRecordField(int rindex, int findex) const;

	void setAllRecords(const OGRLayer *layer);
	void setAllRecords(const char * bytes);
	void setAllRecords(const LayerAllRecords & allrecords);

private:
	typedef enum {
		UNINITIALIZED, STALE, LATEST
	} BufferFlagType;

	void operator=(const LayerAllRecords &);

	int recordlength_;
	int recordcount_;
	int fieldcount_;

	LayerRecordField * fields_;

	char *buffer_;
	BufferFlagType bufferflag_;
};

#endif /* LAYERALLRECORDS_H_ */

/// @file sctest.cc
/// @author luliang@ict.ac.cn
/// @copybrief Copyright 2012 ICT, CAS. All rights reserved.
/// @version 0.6
/// @date 2013-06-18

#include <stdio.h>

#include "spatialClient.h"



int main()
{
	SpatialClient client;
	if(client.connect()) {

	}
	else {
		fprintf(stderr, "Can not connect the redis server.\n");
	}
	return 0;
}


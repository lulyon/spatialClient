/*
 * sctest.cc
 *
 *  Created on: 2013-6-18
 *      Author: luliang
 */


int main()
{
  SpatialClient client;
	if(client.connect()) {

	}
	else {
		fprintf(stderr, "Can't connect the redis server.\n");
	}
	return 0;
}


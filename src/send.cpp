/*
 * send.cpp

 *
 *  Created on: 18 juil. 2017
 *      Author: fab
 */

#include <iostream>

using namespace std;

struct t_ramdrive {
	char * name ;
	int size ;
	unsigned char  * data ;

};

typedef struct  {

	unsigned char id ;
	char * name ;
	struct t_ramdrive ramdrive ;



} user ;
user my_user ;
#define OFDM_SIZE 512
int g_numberOfUsers = 23 ;

unsigned char gOfdmSymbolBuffer[OFDM_SIZE] ;


bool send ( user u[g_numberOfUsers ] )
{
	bool ret = false ;

	int i=0 ;
	int headerSize =0 ;

	/* get header size */
	while (i < g_numberOfUsers)
	{
		headerSize += u[g_numberOfUsers].ramdrive.size ;
		i++;

	}


	/* fill header */
	gOfdmSymbolBuffer[0] = headerSize ;
	i = 1 ;
	while (i < g_numberOfUsers )
	{
		gOfdmSymbolBuffer[i]=headerSize + u[i-1].ramdrive.size;
		i++;
	}

	/* fill data */

	int spaceToFillDataSize = (OFDM_SIZE*8) -  headerSize ;
	int NbBytePerUser = std::abs(spaceToFillDataSize / g_numberOfUsers );

	i = 0 ;
	while (i < NbBytePerUser)
	{
		memcpy(gOfdmSymbolBuffer +headerSize ,u[i].ramdrive.data,  NbBytePerUser) ;
		i++ ;

	}



	return ret ;
}


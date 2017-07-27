/*
 * send.cpp

 *
 *  Created on: 18 juil. 2017
 *      Author: fab
 */

#include <iostream>
#include <string.h>

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
#define BYTE_PER_USER 5
unsigned char gOFDMbuffer[OFDM_SIZE] ;
int g_numberOfUsers = 23 ;
unsigned char *data ;

short gIdNextLocation ; //where do i add in gDATA_2_SEND_TAB


typedef  struct
{
	unsigned char data  [250] ;
	int size ;
} DATA2SEND ;


/* les données a envoyer sont stockés ici ..*/
int gDataToSendIndex = 0 ; //index du numero de data à traiter
DATA2SEND gDATA_2_SEND_TAB[16] ;

int gCurrentIndex = 0 ;

/*function */
void addDataToOfdm(  int , unsigned char * , int) ;



unsigned char gOfdmSymbolBuffer[OFDM_SIZE] ;





/*

Add data to OFDM array

 */
void addDataToOFDM (unsigned char * data , int nb)
{
	memcpy(gDATA_2_SEND_TAB +gCurrentIndex, (const void *)data, sizeof(unsigned char )*nb) ;
	gIdNextLocation= (gIdNextLocation>16) ? 0: gCurrentIndex++  ;

}


/*
 *
 *
 *
 * get nb bytes from interface id..
 */
void getData2send (void * data,int nbWanted ,  int* nbReceived , int id)
{




}

bool gAlive = true ;
void sender_loop ()
{

	int room = std::abs(OFDM_SIZE /8 );
	int id =0 ; //cureent interface
	int nb ;
	int bytePerInterface = 5 ;
	while (gAlive)
	{

		/* tant qu'il y a de la place et tant qu'on a pas attendu trop longtemps ..*/
		/*tant qu'il y a des interface */
		//regarder si il y a des datas a envoyer
		//si il y a des data, lire nb data suivant la politique


		while  (room > 0 )
		{





					nb = (gDATA_2_SEND_TAB[gCurrentIndex].size >  BYTE_PER_USER ) ? gDATA_2_SEND_TAB[gCurrentIndex].size : BYTE_PER_USER ;

					if (gDATA_2_SEND_TAB[gCurrentIndex].size > BYTE_PER_USER)
						gDATA_2_SEND_TAB[gCurrentIndex].size = gDATA_2_SEND_TAB[gCurrentIndex].size - BYTE_PER_USER ;

					addDataToOfdm(  id, gDATA_2_SEND_TAB[gCurrentIndex].data,nb) ;

					room-=BYTE_PER_USER ;







				gCurrentIndex++ ;


		}





	}







}


void addDataToOfdm(  int id, unsigned char *  data, int nb)
{

	/* premier byte de la zone data donne le nombre de paquets envoyés */
	gOFDMbuffer[id] = gOFDMbuffer[id-1] + gOFDMbuffer[gOFDMbuffer[id-1]] ;
	memcpy(gOFDMbuffer + gOFDMbuffer[id] , data , nb) ;


}


bool send ( user u[g_numberOfUsers ] )
{
	bool ret = false ;

	int i=0 ;
	int headerSize =0 ;

	/* get header size */
	//while (i < g_numberOfUsers)
	//{
	//	headerSize += u[g_numberOfUsers].ramdrive.size ;
	//	i++;

	//}
	headerSize = g_numberOfUsers * sizeof(unsigned char) ;



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


/*
 *
 *
 *
 * ici on recupere les données de l'interface reseau à envoyer dans ofdm
 */




/*
 *
 * useless
 *
 */
void getData2send (unsigned char * data, int bytePerInterface ,  int *nb , int  id)
{

	data =gDATA_2_SEND_TAB[gCurrentIndex].data ;
	*nb = (gDATA_2_SEND_TAB[gCurrentIndex].size < bytePerInterface ) ? gDATA_2_SEND_TAB[gCurrentIndex].size : bytePerInterface ;




}

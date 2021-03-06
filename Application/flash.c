#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include "nordic_common.h"
#include "nrf_error.h"
#include "nrf_assert.h"
#include "nrf.h"
#include "nrf_soc.h"
#include "app_util.h"
#include "boards.h"
#include "ble_flash.h"
#include "pm.h"
#include "app_error.h"
#include "pstorage.h"
#include "uart.h"
//#include "state_machine.h"
#include "flash.h"


#if (FLASH_DATA_LAPA_EN==true)
static volatile uint8_t pstorage_wait_flag = 0;
static volatile pstorage_block_t pstorage_wait_handle = 0;
pstorage_handle_t       handle;
static pstorage_handle_t		block_0_handle;
static volatile uint8_t flash_state=0;
static volatile uint8_t flash_wait_state=0;

static DEVICE flash_data_rd;
#define FLASH_SBUFF	10
static DEVICE flash_data_wr[FLASH_SBUFF];
static volatile uint8_t flash_wr_counter=0;
static volatile uint8_t flash_wr_wr_ptr=0;
static volatile uint8_t flash_wr_rd_ptr=0;

static uint8_t *ptr;
static DEVICE tmp;
static uint8_t buffer_data[16];





static bool ram_flash_flag=false;
static DEVICE mem_dev;

bool flash_push(DEVICE dev)
{
	if(flash_wr_counter>=FLASH_SBUFF)
		return false;
	memcpy(flash_data_wr+flash_wr_wr_ptr, &dev, sizeof(DEVICE));
	flash_wr_wr_ptr++;
	if(flash_wr_wr_ptr>=FLASH_SBUFF)
		flash_wr_wr_ptr=0;
	flash_wr_counter++;
	return true;
}


bool flash_pop(DEVICE *dev)
{
	if(flash_wr_counter==0)
		return false;
	flash_wr_counter--;
	memcpy(dev,flash_data_wr+flash_wr_rd_ptr,  sizeof(DEVICE));
	flash_wr_rd_ptr++;
	if(flash_wr_rd_ptr>=FLASH_SBUFF)
		flash_wr_rd_ptr=0;
	return true;
}


uint8_t flash_num_elems_in_buffer(void)
{
	return  flash_wr_counter;
}
#endif
#if (FLASH_DATA_LAPA_EN==true)
/*


void flash_clear_flag(pstorage_handle_t block_handle)
{
    pstorage_wait_handle = block_handle.block_id;            //Specify which pstorage handle to wait for
    pstorage_wait_flag = 0;
}

void flash_waiting_for_state(uint8_t state)
{
	flash_clear_flag(block_0_handle);
	flash_wait_state=state;
}


bool flash_ready(void)
{

	if(pstorage_wait_flag)
		if(flash_state==flash_wait_state)
			return true;
	return false;

}

uint8_t flash_get_status(void)
{
	return flash_state;
}


static void flash_data_cb_handler(pstorage_handle_t  * handle, uint8_t  op_code,  uint32_t result, uint8_t *p_data, uint32_t  data_len)
{
		if(handle->block_id == pstorage_wait_handle)
			pstorage_wait_flag = 1; 					//If we are waiting for this callback, set the wait flag.

		switch(op_code)
		{
			case PSTORAGE_LOAD_OP_CODE:
				 if (result == NRF_SUCCESS)
				 {
					 	flash_state=FLASH_STATE_LOAD_OK;
						#if (UART_EN==true)
						 printf("pstorage LOAD callback received \r\n");
						// bsp_indication_set(BSP_INDICATE_ALERT_0);
						#endif
				 }
				 else
				 {
						#if (UART_EN==true)
						 printf("pstorage LOAD ERROR callback received \r\n");
						#endif
						 	flash_state=FLASH_STATE_LOAD_ERR;

						 //bsp_indication_set(BSP_INDICATE_RCV_ERROR);
				 }
				 break;
			case PSTORAGE_STORE_OP_CODE:
				 if (result == NRF_SUCCESS)
				 {
					 	flash_state=FLASH_STATE_STORE_OK;

						#if (UART_EN==true)
						 printf("pstorage STORE callback received \r\n");
						#endif

						 //bsp_indication_set(BSP_INDICATE_ALERT_1);
				 }
				 else
				 {
					 	flash_state=FLASH_STATE_STORE_ERR;

						#if (UART_EN==true)
					   printf("pstorage STORE ERROR callback received \r\n");
						#endif

						// bsp_indication_set(BSP_INDICATE_RCV_ERROR);
				 }
				 break;
			case PSTORAGE_UPDATE_OP_CODE:
				 if (result == NRF_SUCCESS)
				 {
					 	flash_state=FLASH_STATE_UPDATE_OK;

						#if (UART_EN==true)
						 printf("pstorage UPDATE callback received \r\n");
						#endif

						 //bsp_indication_set(BSP_INDICATE_ALERT_2);
				 }
				 else
				 {
					 	flash_state=FLASH_STATE_UPDATE_ERR;
						#if (UART_EN==true)
						 printf("pstorage UPDATE ERROR callback received \r\n");
						#endif
						 //bsp_indication_set(BSP_INDICATE_RCV_ERROR);
				 }
				 break;
			case PSTORAGE_CLEAR_OP_CODE:
				 if (result == NRF_SUCCESS)
				 {
					 	flash_state=FLASH_STATE_CLEAR_OK;
						#if (UART_EN==true)
						 printf("pstorage CLEAR callback received \r\n");
						#endif

						 //bsp_indication_set(BSP_INDICATE_ALERT_3);
				 }
				 else
				 {
					 	flash_state=FLASH_STATE_CLEAR_ERR;
						#if (UART_EN==true)
					   printf("pstorage CLEAR ERROR callback received \r\n");
						#endif

						 //bsp_indication_set(BSP_INDICATE_RCV_ERROR);
				 }
				 break;

		}
}


void init_flash_test(void)
{
	uint32_t err_code;
pstorage_module_param_t param;


// Initialize persistent storage module.
err_code = pstorage_init();
APP_ERROR_CHECK(err_code);

param.block_size  = 16;                   //Select block size of 16 bytes
param.block_count = 1;                   //Select 10 blocks, total of 160 bytes
param.cb          = flash_data_cb_handler;   //Set the pstorage callback handler

printf("\r\n\r\npstorage initializing ... \r\n");
err_code = pstorage_register(&param, &handle);
APP_ERROR_CHECK(err_code);

//Get block identifiers
pstorage_block_identifier_get(&handle, 0, &block_0_handle);

}
*/
#endif

/*
bool flash_write_test(DEVICE dev)
{
	uint8_t i;
	uint8_t *ptr;
	uint32_t err_code;

//    flash_clear_flag(block_0_handle);
	flash_waiting_for_state(FLASH_STATE_CLEAR_OK);
	err_code = pstorage_clear(&block_0_handle, 16);
    if(err_code != NRF_SUCCESS)
    	printf("\r\n\r\npstorage error clear block: 0 \r\n");


    while(!flash_ready())
    {   err_code = sd_app_evt_wait();
    	APP_ERROR_CHECK(err_code);
    }
    if(flash_state!=FLASH_STATE_CLEAR_OK)
    	return false;



//    flash_clear_flag(block_0_handle);
	flash_waiting_for_state(FLASH_STATE_STORE_OK);
	ptr=(uint8_t *)&dev;
	for(i=0;i<sizeof(DEVICE);i++)
	{
	  	printf("%u, ",*ptr);
		buffer_data[i]=*(ptr++);

	}
	err_code=pstorage_store(&block_0_handle, buffer_data, 16, 0);     //Write to flash
	if(err_code != NRF_SUCCESS)
		printf("\r\n\r\npstorage error writing data to block 0\r\n");
	return true;
}
*/
/*
bool flash_read_test(DEVICE *dev)
{
	uint8_t *ptr;
	uint32_t err_code;
	uint8_t i;

    printf("pstorage wait for block 0 store to complete ... \r\n");

    while(flash_ready())
    {   err_code = sd_app_evt_wait();
    	APP_ERROR_CHECK(err_code);
    }

	flash_waiting_for_state(FLASH_STATE_LOAD_OK);

	ptr=(uint8_t *)dev;
	memset(buffer_data,0,sizeof(buffer_data) );
    pstorage_load(buffer_data, &block_0_handle,  16, 0);				 //Read from flash, only one block is allowed for each pstorage_load command
    for(i=0;i<sizeof(DEVICE);i++)
    {
    	*ptr=buffer_data[i];
    	ptr++;
    	  printf("%u, ",buffer_data[i]);
    }

    while(flash_ready())
    {   err_code = sd_app_evt_wait();
    	APP_ERROR_CHECK(err_code);
    }
#if (UART_EN==true)
 printf("ready \r\n");
#endif

    if(dev->state==STATE_LAPA_UNREGISTERED)
    	return false;

	return true;
}

*/
#if (FLASH_DATA_LAPA_EN==true)

bool flash_write_setup(DEVICE dev)
{
	uint8_t *ptr;
	uint8_t tmp;
	uint32_t * addr;
    uint32_t   pg_size;
    uint32_t   pg_num;
    uint8_t i;

    ram_flash_flag=false;

    pg_size = NRF_FICR->CODEPAGESIZE;
    pg_num  = BLE_FLASH_PAGE_END-3;						//!!!3 páginas abaixo do bootloader as 2 páginas abaixo do bootloader estão reservadas ás comunicações persistentes!!!
	addr= (uint32_t *) (pg_size*pg_num);
	ble_flash_page_erase(pg_num);
	tmp='P';
	ble_flash_word_write(addr, (uint32_t)tmp);
	ptr=(uint8_t *)&dev;
	for(i=0;i<sizeof(DEVICE);i++)
		ble_flash_word_write(++addr, (uint32_t)(*(ptr++)));

	memcpy(&mem_dev,&dev,sizeof(DEVICE));
	return true;
}

bool flash_write_setup_mem(DEVICE dev)
{
	ram_flash_flag=true;
	memcpy(&mem_dev,&dev,sizeof(DEVICE));
	return true;
}

bool flash_read_setup(DEVICE *dev)
{
	uint8_t  *ptr;
	uint32_t *addr;
    uint32_t pg_size;
    uint32_t pg_num;
    uint8_t  i;

    if(!ram_flash_flag)
    {
		pg_size = NRF_FICR->CODEPAGESIZE;
		pg_num  = BLE_FLASH_PAGE_END-3;  					//!!!3 páginas abaixo do bootloader as 2 páginas abaixo do bootloader estão reservadas ás comunicações persistentes!!!
		addr= (uint32_t *) (pg_size*pg_num);
		if(((uint8_t)*addr)!='P')
			return false;
		ptr=(uint8_t *)dev;
		for(i=0;i<sizeof(DEVICE);i++)
		{
			*ptr=(uint8_t)*(++addr);
			ptr++;
		}
		memcpy(&mem_dev,&dev,sizeof(DEVICE));
    }
    else													//If there are data unsafe in flash memmory
    	memcpy(dev,&mem_dev,sizeof(DEVICE));

	return true;

}

bool flash_is_data_unsafe(void)
{
	return ram_flash_flag;
}


void flash_set_defaults(void)
{
	DEVICE dev;
	#if (DEBUG==true)
	dev.state=STATE_LAPA_UNREGISTERED;
	#else
	dev.state=STATE_LAPA_POFF;
	#endif
//	#ifdef _TEST_GATEWAY_
//	dev.state=STATE_LAPA_UNREGISTERED;
//	#endif
	dev.adv2.enable=ADVERTISE_ENABLE;
	dev.adv2.time_adv=40;	//40 ->4.0 Seg
	flash_write_setup(dev);
	flash_read_setup(&dev);
}

/*
void flash_app_loop(void)
{
	uint32_t err_code;
	uint8_t i;

	switch(flash_st.cstate)
	{
		case 0:
			if(flash_num_elems_in_buffer()>0)
				flash_st.nxstate=1;
			break;

		case 1:
			flash_st.nxstate=2;

			flash_waiting_for_state(FLASH_STATE_CLEAR_OK);
			err_code = pstorage_clear(&block_0_handle, 16);
		    if(err_code != NRF_SUCCESS)
		    	printf("\r\n\r\npstorage error clear block: 0 \r\n");
			break;
		case 2:
		   if(flash_ready())
				flash_st.nxstate=3;

			break;

		case 3:
			flash_st.nxstate=4;
				flash_waiting_for_state(FLASH_STATE_STORE_OK);
				flash_pop(&tmp);
				ptr=(uint8_t *)&tmp;
				for(i=0;i<sizeof(DEVICE);i++)
				{
				  //	printf("%u, ",*ptr);
					buffer_data[i]=*(ptr++);
				}
				err_code=pstorage_store(&block_0_handle, buffer_data, 16, 0);     //Write to flash
				if(err_code != NRF_SUCCESS)
					printf("\r\n\r\npstorage error writing data to block 0\r\n");
			break;
		case 4:
		   if(flash_ready())
				flash_st.nxstate=5;
			break;

		case 5:
			flash_st.nxstate=6;
			flash_waiting_for_state(FLASH_STATE_LOAD_OK);
			ptr=(uint8_t *)&flash_data_rd;
			//memset(buffer_data,0,sizeof(buffer_data));
		    pstorage_load(buffer_data, &block_0_handle,  16, 0);				 //Read from flash, only one block is allowed for each pstorage_load command
		    for(i=0;i<sizeof(DEVICE);i++)
		    {
		    	*ptr=buffer_data[i];
		    	 printf("%u, ",*ptr);
		    	ptr++;
		    }


			break;
		case 6:
		   if(flash_ready())
		   {
				#if (UART_EN==true)
				printf("ready \r\n");
				#endif
				if(flash_data_rd.state==STATE_LAPA_POFF)
	    			NVIC_SystemReset();
				flash_st.nxstate=0;
		   }
			break;
		case 7:

			break;
		default:
			//flash_st.nxstate=0;
			break;
	}
	flash_st.pvstate=flash_st.cstate;
	flash_st.cstate=flash_st.nxstate;
}
*/
/*
bool flash_read_setup(DEVICE *dev)
{

	memcpy(dev,&flash_data_rd,sizeof(DEVICE));
	return true;
}
*/
/*

void flash_set_defaults(void)
{
	DEVICE dev;
	dev.state=STATE_LAPA_POFF;

	dev.adv2.enable=ADVERTISE_ENABLE;
	dev.adv2.time_adv=40;	//40 ->4.0 Seg


	flash_push(dev);

	while(1)
		flash_app_loop();
}
*/
#endif




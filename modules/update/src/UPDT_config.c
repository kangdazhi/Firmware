/* Copyright 2015, Daniel Cohen
 * Copyright 2015, Esteban Volentini
 * Copyright 2015, Matias Giori
 * Copyright 2015, Franco Salinas
 * Copyright 2015, Pablo Alcorta
 *
 * This file is part of CIAA Firmware.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */

/** \brief This file implements the UPDT unpacker functionality
 **/

/** \addtogroup CIAA_Firmware CIAA Firmware
 ** @{ */
/** \addtogroup UPDT CIAA UPDT Unpacker
 ** @{ */

/*
 * Initials     Name
 * ---------------------------
 * DC           Daniel Cohen
 * EV           Esteban Volentini
 * MG           Matias Giori
 * FS           Franco Salinas
 * PA           Pablo Alcorta
 */

/*
 * modification history (new versions first)
 * -----------------------------------------------------------
 * 20150515 v0.0.1  FS  first initial version
 */

/*==================[inclusions]=============================================*/
#include "UPDT_osal.h"
#include "UPDT_config.h"
#include "UPDT_utils.h"

/*==================[macros and definitions]=================================*/
/** \brief Flash memory address where the configuration is stored */
#define UPDT_CONFIG_ADDRESS      0x0

/** \brief Configuration size */
#define UPDT_CONFIG_SIZE         0x18

typedef struct
{
   uint32_t reserved1;
   uint32_t firmware_version;
   uint32_t bootloader_flags;
   uint32_t bootloader_version;
   uint32_t reserved2;
   uint32_t application_version;
   uint32_t vendor_id;
   uint32_t model_id;
   uint64_t unique_id;
   uint32_t data_size;
} UPDT_configType;
/*==================[internal data declaration]==============================*/

/*==================[internal functions declaration]=========================*/

/*==================[internal data definition]===============================*/
static UPDT_configType UPDT_config_new;
static UPDT_configType UPDT_config_old;
static uint8_t UPDT_config_buffer[UPDT_CONFIG_SIZE];
static int32_t UPDT_config_fd;
static uint32_t UPDT_config_flags;
/*==================[external data definition]===============================*/

/* payload parsing */
/*==================[internal functions definition]==========================*/
void UPDT_configParse(const uint8_t *payload, size_t size, UPDT_configType *info)
{
   uint32_t word;
   const uint32_t *ptr;
   assert(NULL != payload);
   assert(NULL != info);

   ptr = (const uint32_t *) payload;

   word = UPDT_utilsNtohl(*ptr);
   info->reserved1 = 0xFFu & (word >> 24);
   info->firmware_version = 0xFFFFFFu & (word);

   word = UPDT_utilsNtohl(*(ptr + 1));
   info->bootloader_flags = 0xFFu & (word >> 24);
   info->bootloader_version = 0xFFFFFFu & (word);

   word = UPDT_utilsNtohl(*(ptr + 2));
   info->reserved2 = 0xFFu & (word >> 24);
   info->application_version = 0xFFFFFFu & (word);

   word = UPDT_utilsNtohl(*(ptr + 3));
   info->vendor_id = 0xFFu & (word >> 24);
   info->model_id = 0xFFFFFFu & (word);

   info->unique_id = UPDT_utilsNtohll(*(const uint64_t *) (ptr + 4));

   info->data_size = UPDT_utilsNtohl(*(ptr + 6));
}

/** \brief UPDT_configFormat stores in a buffer, in compacted format, the parameters of to data structure
 ** \param config_buffer is the buffer wherein the content of the structure is stored
 ** \param data_size is the number of elements that will have the buffer
 ** \param info is a variable of the type structure used to assemble the buffer
 **/
static void UPDT_configFormat (uint8_t *config_buffer,size_t data_size, UPDT_configType *info)
{
   ciaaPOSIX_assert(data_size == 28);
   /*Set of the field "reserved1" in the buffer*/
   config_buffer[0] = info->reserved1;
   /*Set of the field "firmware_version" in the buffer*/
   config_buffer[1] = (info->firmware_version) >> 16;
   config_buffer[2] = (info->firmware_version) >> 8;
   config_buffer[3] = info->firmware_version;
   /*Set of the field "bootloader_flags" in the buffer*/
   config_buffer[4] = info->bootloader_flags;
   /*Set of the field "bootloader_version" in the buffer*/
   config_buffer[5] = (info->bootloader_version) >> 16;
   config_buffer[6] = (info->bootloader_version) >> 8;
   config_buffer[7] = info->bootloader_version;
   /*Set of the field "reserved2" in the buffer*/
   config_buffer[8] = info->reserved2;
   /*Set of the field "application_version" in the buffer*/
   config_buffer[9] = (info->application_version) >> 16;
   config_buffer[10] = (info->application_version) >> 8;
   config_buffer[11] = info->application_version;
   /*Set of the field "vendor_id" in the buffer*/
   config_buffer[12] = info->vendor_id;
   /*Set of the field "model_id" in the buffer*/
   config_buffer[13] = (info->model_id) >> 16;
   config_buffer[14] = (info->model_id) >> 8;
   config_buffer[15] = info->model_id;
   /*Set of the field "unique_id" in the buffer*/
   config_buffer[16] = (info->unique_id) >> 56;
   config_buffer[17] = (info->unique_id) >> 48;
   config_buffer[18] = (info->unique_id) >> 40;
   config_buffer[19] = (info->unique_id) >> 32;
   config_buffer[20] = (info->unique_id) >> 24;
   config_buffer[21] = (info->unique_id) >> 16;
   config_buffer[22] = (info->unique_id) >> 8;
   config_buffer[23] = info->unique_id;
   /*Set of the field "data_size" in the buffer*/
   config_buffer[24] = (info->data_size) >> 24;
   config_buffer[25] = (info->data_size) >> 16;
   config_buffer[26] = (info->data_size) >> 8;
   config_buffer[27] = info->data_size;
}
/*==================[external functions definition]==========================*/

/** \note THE BOOTLOADER FLAGS AND VERSIONS ARE NOT STORED WITH A STORE CALL.
 ** BOTH MUST BE CONSTANTS DEFINED IN THE SERVICE HEADER FILE. A FIRMWARE
 ** UPDATE CANNOT CHANGE THE BOOTLOADER ATTRIBUTES.
 **/
uint32_t UPDT_configSet(const uint8_t *config, size_t size)
{
   /* compare the expected configuration size with the received size */
   if(UPDT_CONFIG_SIZE != size)
   {
      UPDT_config_flags = UPDT_CONFIG_ERROR_INVALID_SIZE;
   }
   else
   {
      UPDT_configParse(config, UPDT_CONFIG_SIZE, &UPDT_config_new);
      UPDT_configParse(UPDT_CONFIG_ADDRESS, UPDT_CONFIG_SIZE, &UPDT_config_old);

      /* ERRORS */

      /* reserved fields must be zero */
      if(0 != UPDT_config_new.reserved1 | UPDT_config_new.reserved2)
      {
         UPDT_config_flags |= UPDT_CONFIG_ERROR_RESERVED;
      }

      if (UPDT_config_new.bootloader_flags != 0)
      {
         UPDT_config_flags |= UPDT_CONFIG_ERROR_BOOTLOADER_FLAGS;
      }

      if (UPDT_config_old.bootloader_version > UPDT_config_new.application_version)
      {
         UPDT_config_flags |= UPDT_CONFIG_ERROR_BOOTLOADER_VERSION;
      }

      if (UPDT_config_old.vendor_id != UPDT_config_new.vendor_id)
      {
         UPDT_config_flags |= UPDT_CONFIG_ERROR_VENDOR_ID;
      }

      if (UPDT_config_old.model_id != UPDT_config_new.model_id)
      {
         UPDT_config_flags |= UPDT_CONFIG_ERROR_MODEL_ID;
      }

      if (UPDT_config_old.unique_id != UPDT_config_new.unique_id)
      {
         UPDT_config_flags |= UPDT_CONFIG_ERROR_UNIQUE_ID;
      }

      /* WARNINGS */

      if (UPDT_config_old.firmware_version >= UPDT_config_new.firmware_version)
      {
         UPDT_config_flags |= UPDT_CONFIG_WARNING_FIRMWARE_VERSION;
      }

      if (UPDT_config_old.application_version >= UPDT_config_new.application_version)
      {
         UPDT_config_flags |= UPDT_CONFIG_WARNING_APPLICATION_VERSION;
      }
   }
   return ret;
}



int32_t UPDT_configWrite(void)
{
   int32_t ret = -1;
   ciaaPOSIX_assert(UPDT_config_fd >= 0);

   /*To the previous configuration is assigned the new version of protocol and firmware, because
   it's the only change (the other parameters are kept)*/
   UPDT_config_old->firmware_version = UPDT_config_new->firmware_version;
   UPDT_config_old->application_version = UPDT_config_new->application_version;

   /*Becomes the structure in an bytes sequence*/
   UPDT_configFormat(UPDT_config_buffer,28,UPDT_config_old);

   /*function call to move the point to the memory address where it will be written*/
   if (ciaaPOSIX_lseek (UPDT_config_fd,UPDT_CONFIG_ADDRESS,SEEK_SET) == UPDT_CONFIG_ADDRES);
   {
      ret = ciaaPOSIX_write (UPDT_config_fd,UPDT_config_buffer,28);
   }
   return ret;
}


int32_t UPDT_configInit (int32_t fd)
{
   UPDT_config_fd = fd;
   return 0;
}


void UPDT_configClear (void)
{
   UPDT_config_fd = -1;
}


ssize_t UPDT_configSetResponse(const uint8_t *buffer, size_t size)
{
   UPDT_configFormat(buffer,size,UPDT_config_old);
   UPDT_config_flags = UPDT_utilsNtohl (UPDT_config_flags);

   /* Concatenated flags of warning and error */
   buffer[size] = UPDT_config_flags >> 24;
   buffer[size+1] = UPDT_config_flags >> 16;
   buffer[size+2] = UPDT_config_flags >> 8;
   buffer[size+3] = UPDT_config_flags;
   return 0;
}
/** @} doxygen end group definition */
/** @} doxygen end group definition */
/*==================[end of file]============================================*/


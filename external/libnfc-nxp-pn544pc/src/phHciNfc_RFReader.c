/*
 * Copyright (C) 2010-2013 NXP Semiconductors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


/*!
* =========================================================================== *
*                                                                             *
*                                                                             *
* \file  phHciNfc_RFReader.c                                                  *
* \brief HCI RF Reader Management Gate Routines.                              *
*                                                                             *
*                                                                             *
* Project: NFC-FRI-1.1                                                        *
*                                                                             *
* $Date: Thu Dec 23 17:00:05 2010 $                                           *
* $Author: ravindrau $                                                         *
* $Revision: 1.55 $                                                            *
* $Aliases:  $                                                                *
*                                                                             *
*
* $Developed by : Ravindra U   $                                              *
*
*                                                                             *
* =========================================================================== *
*/

/*
***************************** Header File Inclusion ****************************
*/
#include <phNfcConfig.h>
#include <phNfcCompId.h>
#include <phHciNfc_Pipe.h>
#include <phHciNfc_RFReader.h>
#include <phHciNfc_RFReaderA.h>
#ifdef TYPE_B
#include <phHciNfc_RFReaderB.h>
#endif
#ifdef ENABLE_P2P
#include <phHciNfc_NfcIPMgmt.h>
#endif
#ifdef TYPE_FELICA
#include <phHciNfc_Felica.h>
#endif
#ifdef TYPE_JEWEL
#include <phHciNfc_Jewel.h>
#endif
#ifdef TYPE_ISO15693
#include <phHciNfc_ISO15693.h>
#endif /* #ifdef    TYPE_ISO15693 */
#include <phOsalNfc.h>

/*
****************************** Macro Definitions *******************************
*/

#define NFCIP_ACTIVATE_DELAY            0x05U
#define TIMEOUTACTIVATE(X)              ((X)|(0x10))
#define NFC_HCI_EXTND_ISO_MULTIPLIER    0x14U
/*
*************************** Structure and Enumeration ***************************
*/


/** \defgroup grp_hci_nfc HCI Reader RF Management Component
 *
 *
 */


typedef enum phHciNfc_ReaderMgmt_Seq{
    READERA_PIPE_OPEN       = 0x00U,
    READERA_SET_DATA_RATE,
    READERA_DISABLE_AUTO_ACTIVATE,
    READERB_PIPE_OPEN,
    FELICA_PROP_PIPE_OPEN,
    JEWEL_PROP_PIPE_OPEN,
    ISO15693_PROP_PIPE_OPEN,
    NFCIP1_INITIATOR_PIPE_OPEN,
    NFCIP1_INITIATOR_MODE_CONFIG,
    NFCIP1_INITIATOR_PSL1_CONFIG,
    NFCIP1_INITIATOR_PSL2_CONFIG,
    NFCIP1_INITIATOR_DID_CONFIG,


    READERA_PIPE_CLOSE,
    READERB_PIPE_CLOSE,
    FELICA_PROP_PIPE_CLOSE,
    JEWEL_PROP_PIPE_CLOSE,
    ISO15693_PROP_PIPE_CLOSE,
    NFCIP1_INITIATOR_PIPE_CLOSE,
    END_READER_SEQUENCE
} phHciNfc_ReaderMgmt_Seq_t;

typedef struct phHciNfc_ReaderMgmt_Info{
    phHciNfc_ReaderMgmt_Seq_t   rf_gate_cur_seq;
    phHciNfc_ReaderMgmt_Seq_t   rf_gate_next_seq;
} phHciNfc_ReaderMgmt_Info_t;


/*
*************************** Static Function Declaration **************************
*/
static
NFCSTATUS
phHciNfc_ReaderMgmt_End_Discovery(
                                phHciNfc_sContext_t     *psHciContext,
                                void                    *pHwRef,
                                uint8_t                 reader_pipe_id
                             );

static
NFCSTATUS
phHciNfc_ReaderMgmt_Initiate_Discovery(
                                phHciNfc_sContext_t     *psHciContext,
                                void                    *pHwRef,
                                uint8_t                 reader_pipe_id
                             );

/*
*************************** Function Definitions ***************************
*/

#ifdef READER_INIT
/*!
 * \brief Allocates the resources of RF Reader Managment Gate.
 *
 * This function Allocates the resources of the RF Reader Management
 * gate Information Structure.
 *
 */

NFCSTATUS
phHciNfc_ReaderMgmt_Init_Resources(
                                phHciNfc_sContext_t     *psHciContext
                             )
{
    NFCSTATUS                   status = NFCSTATUS_SUCCESS;
    phHciNfc_ReaderMgmt_Info_t  *p_reader_mgmt_info=NULL;
    if( NULL == psHciContext )
    {
        status = PHNFCSTVAL(CID_NFC_HCI, NFCSTATUS_INVALID_PARAMETER);
    }
    else
    {
        if( ( NULL == psHciContext->p_reader_mgmt_info )
            && (phHciNfc_Allocate_Resource((void **)(&p_reader_mgmt_info),
                    sizeof(phHciNfc_ReaderMgmt_Info_t))== NFCSTATUS_SUCCESS)
        )
        {
            psHciContext->p_reader_mgmt_info = p_reader_mgmt_info;
            p_reader_mgmt_info->rf_gate_cur_seq = READERA_PIPE_OPEN;
            p_reader_mgmt_info->rf_gate_next_seq = END_READER_SEQUENCE;
        }
        else
        {
            status = PHNFCSTVAL(CID_NFC_HCI, NFCSTATUS_INSUFFICIENT_RESOURCES);
        }
    }
    return status;
}

#endif

/*!
 * \brief Updates the Sequence of RF Reader Managment Gate.
 *
 * This function Resets/Updates the sequence of the RF Reader Management
 * gate.
 *
 */

NFCSTATUS
phHciNfc_ReaderMgmt_Update_Sequence(
                                phHciNfc_sContext_t     *psHciContext,
                                phHciNfc_eSeqType_t     reader_seq
                             )
{
    NFCSTATUS                   status = NFCSTATUS_SUCCESS;
    phHciNfc_ReaderMgmt_Info_t  *p_reader_mgmt_info = NULL;
    if( NULL == psHciContext )
    {
        status = PHNFCSTVAL(CID_NFC_HCI, NFCSTATUS_INVALID_PARAMETER);
    }
    else
    {
        if( NULL == psHciContext->p_reader_mgmt_info )
        {
            status = PHNFCSTVAL(CID_NFC_HCI,
                        NFCSTATUS_INVALID_HCI_INFORMATION);
        }
        else
        {
            p_reader_mgmt_info = (phHciNfc_ReaderMgmt_Info_t *)
                                psHciContext->p_reader_mgmt_info ;
            switch(reader_seq)
            {
                case RESET_SEQ:
                case INIT_SEQ:
                {
                    p_reader_mgmt_info->rf_gate_cur_seq = READERA_PIPE_OPEN;
                    p_reader_mgmt_info->rf_gate_next_seq = END_READER_SEQUENCE;
                    break;
                }
                case UPDATE_SEQ:
                {
                    p_reader_mgmt_info->rf_gate_cur_seq =
                                            p_reader_mgmt_info->rf_gate_next_seq;
                    break;
                }
                case INFO_SEQ:
                {
                    status = phHciNfc_ReaderA_Update_Info(psHciContext,
                                            HCI_READER_A_INFO_SEQ, NULL);
#if defined( TYPE_B )
                    status = phHciNfc_ReaderB_Update_Info(psHciContext,
                                            HCI_READER_B_INFO_SEQ, NULL);
#endif /* end of #if defined(TYPE_B) */
#if defined( TYPE_FELICA )
                    status = phHciNfc_Felica_Update_Info(psHciContext,
                                            HCI_FELICA_INFO_SEQ, NULL);
#endif /* end of #if defined(TYPE_FELICA) */
#if defined( TYPE_JEWEL )
                    status = phHciNfc_Jewel_Update_Info(psHciContext,
                                            HCI_JEWEL_INFO_SEQ, NULL);
#endif /* end of #if defined(TYPE_JEWEL) */
#if defined( TYPE_ISO15693 )
                    status = phHciNfc_ISO15693_Update_Info(psHciContext,
                                            HCI_ISO_15693_INFO_SEQ, NULL);
#endif /* end of #if defined(TYPE_ISO15693) */
                    break;
                }
                case REL_SEQ:
                {
                    p_reader_mgmt_info->rf_gate_cur_seq = READERA_PIPE_CLOSE;
                    p_reader_mgmt_info->rf_gate_next_seq = END_READER_SEQUENCE;
                    break;
                }
                default:
                {
                    break;
                }
            }
        }
    }

    return status;

}


/*!
 * \brief Initialisation of RF Reader Managment Gate.
 *
 * This function initialses the RF Reader Management gate and
 * populates the Reader Management Information Structure
 *
 */

NFCSTATUS
phHciNfc_ReaderMgmt_Initialise(
                                phHciNfc_sContext_t     *psHciContext,
                                void                    *pHwRef
                         )
{
    NFCSTATUS                       status = NFCSTATUS_SUCCESS;
    phHciNfc_Pipe_Info_t            *p_pipe_info = NULL;
    phHciNfc_ReaderMgmt_Info_t      *p_reader_mgmt_info=NULL;

    if( NULL == psHciContext )
    {
        status = PHNFCSTVAL(CID_NFC_HCI, NFCSTATUS_INVALID_PARAMETER);
    }
    else
    {

        if( ( NULL == psHciContext->p_reader_mgmt_info )
            && (phHciNfc_Allocate_Resource((void **)(&p_reader_mgmt_info),
                    sizeof(phHciNfc_ReaderMgmt_Info_t))== NFCSTATUS_SUCCESS)
        )
        {
            psHciContext->p_reader_mgmt_info = p_reader_mgmt_info;
            p_reader_mgmt_info->rf_gate_cur_seq = READERA_PIPE_OPEN;
            p_reader_mgmt_info->rf_gate_next_seq = END_READER_SEQUENCE;
        }
        else
        {
            p_reader_mgmt_info = (phHciNfc_ReaderMgmt_Info_t *)
                                psHciContext->p_reader_mgmt_info ;
        }

        if( NULL == psHciContext->p_reader_mgmt_info )
        {
            status = PHNFCSTVAL(CID_NFC_HCI,
                                NFCSTATUS_INSUFFICIENT_RESOURCES);
        }
#ifdef ESTABLISH_SESSION
        else if( hciMode_Session == psHciContext->hci_mode )
        {
            status = NFCSTATUS_SUCCESS;
        }
#endif
        else
        {
            switch(p_reader_mgmt_info->rf_gate_cur_seq )
            {
                /* Reader A pipe open sequence */
                case READERA_PIPE_OPEN:
                {
                    p_pipe_info = ((phHciNfc_ReaderA_Info_t *)
                            psHciContext->p_reader_a_info)->p_pipe_info;
                    if(NULL == p_pipe_info )
                    {
                        status = PHNFCSTVAL(CID_NFC_HCI,
                                        NFCSTATUS_INVALID_HCI_SEQUENCE);
                    }
                    else
                    {
                        status = phHciNfc_Open_Pipe( psHciContext,
                                                        pHwRef, p_pipe_info );
                        if(status == NFCSTATUS_SUCCESS)
                        {
                            uint8_t rdr_enable = TRUE;
                            status = phHciNfc_ReaderA_Update_Info(
                                    psHciContext, HCI_READER_A_ENABLE,
                                                            &rdr_enable);
                            p_reader_mgmt_info->rf_gate_next_seq =
                                                        READERA_SET_DATA_RATE;
                            status = NFCSTATUS_PENDING;
                        }
                    }
                    break;
                }
                /* Reader A Set Data Rate to Maximum */
                case READERA_SET_DATA_RATE:
                {
                    p_pipe_info = ((phHciNfc_ReaderA_Info_t *)
                            psHciContext->p_reader_a_info)->p_pipe_info;
                    if(NULL == p_pipe_info )
                    {
                        status = PHNFCSTVAL(CID_NFC_HCI,
                                        NFCSTATUS_INVALID_HCI_SEQUENCE);
                    }
                    else
                    {

                        status = phHciNfc_ReaderA_Set_DataRateMax(
                                        psHciContext, pHwRef,
                                        DATA_RATE_MAX_DEFAULT_VALUE );
                        if(status == NFCSTATUS_PENDING)
                        {
#if defined( TYPE_B )  && defined ( ENABLE_AUTO_ACTIVATE )
                            p_reader_mgmt_info->rf_gate_next_seq =
                                                    READERB_PIPE_OPEN;
/* end of #ifdef TYPE_B */
#elif !defined( ENABLE_AUTO_ACTIVATE )
                            p_reader_mgmt_info->rf_gate_next_seq =
                                                        READERA_DISABLE_AUTO_ACTIVATE;
/* #ifdef ENABLE_AUTO_ACTIVATE */
#elif defined( ENABLE_P2P )
                            p_reader_mgmt_info->rf_gate_next_seq =
                                                    NFCIP1_INITIATOR_PIPE_OPEN;
/* #ifdef ENABLE_P2P */
#else
                            p_reader_mgmt_info->rf_gate_next_seq =
                                                END_READER_SEQUENCE;
                            status = NFCSTATUS_SUCCESS;
#endif
                        }
                    }
                    break;
                }
                /* Reader A Auto Activate Disable */
                case READERA_DISABLE_AUTO_ACTIVATE:
                {
                    uint8_t     activate_enable = FALSE;
                    p_pipe_info = ((phHciNfc_ReaderA_Info_t *)
                            psHciContext->p_reader_a_info)->p_pipe_info;
                    if(NULL == p_pipe_info )
                    {
                        status = PHNFCSTVAL(CID_NFC_HCI,
                                        NFCSTATUS_INVALID_HCI_SEQUENCE);
                    }
                    else
                    {

                        status = phHciNfc_ReaderA_Auto_Activate( psHciContext,
                                                        pHwRef, activate_enable );
                        if(status == NFCSTATUS_SUCCESS)
                        {
#if defined (TYPE_B)
                            p_reader_mgmt_info->rf_gate_next_seq =
                                                    READERB_PIPE_OPEN;
                            status = NFCSTATUS_PENDING;
/* end of #ifdef TYPE_B */
#elif defined(TYPE_FELICA)
                            p_reader_mgmt_info->rf_gate_next_seq =
                                                        FELICA_PROP_PIPE_OPEN;
                            status = NFCSTATUS_PENDING;
/* end of #elif defined(TYPE_FELICA) */
#elif defined(TYPE_JEWEL)
                            p_reader_mgmt_info->rf_gate_next_seq =
                                                        JEWEL_PROP_PIPE_OPEN;
                            status = NFCSTATUS_PENDING;
/* end of #elif defined(TYPE_JEWEL) */
#elif defined (TYPE_ISO15693)
                            p_reader_mgmt_info->rf_gate_next_seq =
                                                    ISO15693_PROP_PIPE_OPEN;
                            status = NFCSTATUS_PENDING;
/* end of #elif defined(TYPE_ISO15693) */
#elif defined(ENABLE_P2P)
                            p_reader_mgmt_info->rf_gate_next_seq =
                                                    NFCIP1_INITIATOR_PIPE_OPEN;
                            status = NFCSTATUS_PENDING;
/* end of #ifdef ENABLE_P2P */
#else
                            p_reader_mgmt_info->rf_gate_next_seq =
                                                            END_READER_SEQUENCE;
                            /* status = NFCSTATUS_PENDING; */
#endif /* #if !defined(ENABLE_P2P) && !defined(TYPE_B)*/
                        }
                    }
                    break;
                }
#ifdef TYPE_B
                /* Reader B pipe open sequence */
                case READERB_PIPE_OPEN:
                {
                    p_pipe_info = ((phHciNfc_ReaderB_Info_t *)
                            psHciContext->p_reader_b_info)->p_pipe_info;
                    if(NULL == p_pipe_info )
                    {
                        status = PHNFCSTVAL(CID_NFC_HCI,
                                        NFCSTATUS_INVALID_HCI_SEQUENCE);
                    }
                    else
                    {
                        status = phHciNfc_Open_Pipe( psHciContext,
                                                        pHwRef, p_pipe_info );
                        if(status == NFCSTATUS_SUCCESS)
                        {
#if defined(TYPE_FELICA)
                            p_reader_mgmt_info->rf_gate_next_seq =
                                                        FELICA_PROP_PIPE_OPEN;
                            status = NFCSTATUS_PENDING;
/* end of #ifdef TYPE_FELICA */
#elif defined(TYPE_JEWEL)
                            p_reader_mgmt_info->rf_gate_next_seq =
                                                        JEWEL_PROP_PIPE_OPEN;
                            status = NFCSTATUS_PENDING;
/* end of #elif defined(TYPE_JEWEL) */
#elif defined (TYPE_ISO15693)
                            p_reader_mgmt_info->rf_gate_next_seq =
                                                    ISO15693_PROP_PIPE_OPEN;
                            status = NFCSTATUS_PENDING;
/* end of #elif defined(TYPE_ISO15693) */
#elif defined(ENABLE_P2P)
                            p_reader_mgmt_info->rf_gate_next_seq =
                                                    NFCIP1_INITIATOR_PIPE_OPEN;
                            status = NFCSTATUS_PENDING;
/* end of #ifdef ENABLE_P2P */
#else
                            p_reader_mgmt_info->rf_gate_next_seq =
                                                            END_READER_SEQUENCE;
                            /* status = NFCSTATUS_PENDING; */
#endif /* #if !defined(ENABLE_P2P) && !defined(TYPE_FELICA)*/
                        }
                    }
                    break;
                }
#endif /* #ifdef TYPE_B */
#ifdef TYPE_FELICA
                /* Felica Reader pipe open sequence */
                case FELICA_PROP_PIPE_OPEN:
                {
                    p_pipe_info = ((phHciNfc_Felica_Info_t *)
                            psHciContext->p_felica_info)->p_pipe_info;
                    if(NULL == p_pipe_info )
                    {
                        status = PHNFCSTVAL(CID_NFC_HCI,
                                        NFCSTATUS_INVALID_HCI_SEQUENCE);
                    }
                    else
                    {
                        status = phHciNfc_Open_Pipe( psHciContext,
                                                        pHwRef, p_pipe_info );
                        if(status == NFCSTATUS_SUCCESS)
                        {
#if defined(TYPE_JEWEL)
                            p_reader_mgmt_info->rf_gate_next_seq =
                                                        JEWEL_PROP_PIPE_OPEN;
                            status = NFCSTATUS_PENDING;
/* end of #if defined(TYPE_JEWEL) */
#elif defined (TYPE_ISO15693)
                            p_reader_mgmt_info->rf_gate_next_seq =
                                                    ISO15693_PROP_PIPE_OPEN;
                            status = NFCSTATUS_PENDING;
/* end of #elif defined(TYPE_ISO15693) */
#elif defined(ENABLE_P2P)
                            p_reader_mgmt_info->rf_gate_next_seq =
                                                    NFCIP1_INITIATOR_PIPE_OPEN;
                            status = NFCSTATUS_PENDING;
 /* end of #ifdef ENABLE_P2P */
#else
                            p_reader_mgmt_info->rf_gate_next_seq =
                                                            END_READER_SEQUENCE;
                            /* status = NFCSTATUS_PENDING; */
#endif /* #if !defined(ENABLE_P2P) */
                        }
                    }
                    break;
                }
#endif
#ifdef TYPE_JEWEL
                /* Jewel Reader pipe open sequence */
                case JEWEL_PROP_PIPE_OPEN:
                {
                    p_pipe_info = ((phHciNfc_Jewel_Info_t *)
                            psHciContext->p_jewel_info)->p_pipe_info;
                    if(NULL == p_pipe_info )
                    {
                        status = PHNFCSTVAL(CID_NFC_HCI,
                                        NFCSTATUS_INVALID_HCI_SEQUENCE);
                    }
                    else
                    {
                        status = phHciNfc_Open_Pipe( psHciContext,
                                                        pHwRef, p_pipe_info );
                        if(status == NFCSTATUS_SUCCESS)
                        {
#if defined (TYPE_ISO15693)
                            p_reader_mgmt_info->rf_gate_next_seq =
                                                    ISO15693_PROP_PIPE_OPEN;
                            status = NFCSTATUS_PENDING;
/* end of #if defined(TYPE_ISO15693) */
#elif defined (ENABLE_P2P)
                            p_reader_mgmt_info->rf_gate_next_seq =
                                                    NFCIP1_INITIATOR_PIPE_OPEN;
                            status = NFCSTATUS_PENDING;
 /* end of #ifdef ENABLE_P2P */
#else
                            p_reader_mgmt_info->rf_gate_next_seq =
                                                            END_READER_SEQUENCE;
                            /* status = NFCSTATUS_PENDING; */
#endif /* #if !defined(ENABLE_P2P) */
                        }
                    }
                    break;
                }
#endif

#ifdef TYPE_ISO15693
                /* ISO15693 Reader pipe open sequence */
                case ISO15693_PROP_PIPE_OPEN:
                {
                    p_pipe_info = ((phHciNfc_ISO15693_Info_t *)
                        psHciContext->p_iso_15693_info)->ps_15693_pipe_info;
                    if(NULL == p_pipe_info )
                    {
                        status = PHNFCSTVAL(CID_NFC_HCI,
                                        NFCSTATUS_INVALID_HCI_SEQUENCE);
                    }
                    else
                    {
                        status = phHciNfc_Open_Pipe( psHciContext,
                                                        pHwRef, p_pipe_info );
                        if(status == NFCSTATUS_SUCCESS)
                        {
#ifdef ENABLE_P2P
                            p_reader_mgmt_info->rf_gate_next_seq =
                                                    NFCIP1_INITIATOR_PIPE_OPEN;
                            status = NFCSTATUS_PENDING;
 /* end of #ifdef ENABLE_P2P */
#else
                            p_reader_mgmt_info->rf_gate_next_seq =
                                                            END_READER_SEQUENCE;
                            /* status = NFCSTATUS_PENDING; */
#endif /* #if !defined(ENABLE_P2P) */
                        }
                    }
                    break;
                }

#endif

#ifdef ENABLE_P2P
                /* NFC-IP1 Initiator pipe open sequence */
                case NFCIP1_INITIATOR_PIPE_OPEN:
                {
                    p_pipe_info =
                        ((phHciNfc_NfcIP_Info_t *)psHciContext->
                                    p_nfcip_info)->p_init_pipe_info;
                    if(NULL == p_pipe_info )
                    {
                        status = PHNFCSTVAL(CID_NFC_HCI,
                                        NFCSTATUS_INVALID_HCI_SEQUENCE);
                    }
                    else
                    {
                        status = phHciNfc_Open_Pipe( psHciContext,
                                                        pHwRef, p_pipe_info );
                        if(status == NFCSTATUS_SUCCESS)
                        {
                            p_reader_mgmt_info->rf_gate_next_seq =
                                                NFCIP1_INITIATOR_MODE_CONFIG;
                            status = NFCSTATUS_PENDING;
                        }
                    }
                    break;
                }
                case NFCIP1_INITIATOR_MODE_CONFIG:
                {
                    uint8_t mode = DEFAULT_NFCIP_MODE_SUPPORT;
                    status = phHciNfc_NfcIP_SetMode( psHciContext, pHwRef,
                                                NFCIP_INITIATOR, mode);
                    if(status == NFCSTATUS_PENDING )
                    {
                        p_reader_mgmt_info->rf_gate_next_seq =
                                            NFCIP1_INITIATOR_PSL1_CONFIG;
                        /* status = NFCSTATUS_SUCCESS; */
                    }
                    break;
                }
                case NFCIP1_INITIATOR_PSL1_CONFIG:
                {
                    uint8_t psl_config = NXP_NFCIP_PSL_BRS_DEFAULT;
                    status = phHciNfc_NfcIP_SetPSL1( psHciContext, pHwRef,
                                                psl_config);
                    if(status == NFCSTATUS_PENDING )
                    {
                        p_reader_mgmt_info->rf_gate_next_seq =
                                             NFCIP1_INITIATOR_DID_CONFIG;
                        /* status = NFCSTATUS_SUCCESS; */
                    }
                    break;
                }
                case NFCIP1_INITIATOR_DID_CONFIG:
                {
                    uint8_t did_config = NXP_NFCIP_DID_DEFAULT;
                    status = phHciNfc_NfcIP_SetDID( psHciContext, pHwRef,
                                                did_config);
                    if(status == NFCSTATUS_PENDING )
                    {
                        p_reader_mgmt_info->rf_gate_next_seq =
                                                        END_READER_SEQUENCE;
                        status = NFCSTATUS_SUCCESS;
                    }
                    break;
                }
#endif
                default:
                {
                    status = PHNFCSTVAL(CID_NFC_HCI, NFCSTATUS_INVALID_HCI_SEQUENCE);
                    break;
                }

            }/* End of the Sequence Switch */

        }/* End of the Reader Info Memory Check */

    } /* End of Null Context Check */

    return status;
}


/*!
 * \brief Initiate the Discovery for the RF Reader .
 *
 * This function starts the Polling Loop and initiates the discovery
 * of the Target.
 *
 */
static
NFCSTATUS
phHciNfc_ReaderMgmt_Initiate_Discovery(
                                phHciNfc_sContext_t     *psHciContext,
                                void                    *pHwRef,
                                uint8_t                 reader_pipe_id
                             )
{
    NFCSTATUS                   status = NFCSTATUS_SUCCESS;

    if( ( NULL == psHciContext )
        || ( NULL == pHwRef )
        || ( HCI_UNKNOWN_PIPE_ID == reader_pipe_id)
        )
    {
        status = PHNFCSTVAL(CID_NFC_HCI, NFCSTATUS_INVALID_PARAMETER);
    }
    else
    {

        status = phHciNfc_Send_RFReader_Event ( psHciContext, pHwRef,
                    reader_pipe_id,(uint8_t) EVT_READER_REQUESTED );
        status = ( (status == NFCSTATUS_PENDING)?
                                NFCSTATUS_SUCCESS : status);
    }
    return status;
}


/*!
 * \brief End the Discovery of the RF Reader .
 *
 * This function stops the Polling Loop and ends the discovery
 * of the Target.
 *
 */
static
NFCSTATUS
phHciNfc_ReaderMgmt_End_Discovery(
                                phHciNfc_sContext_t     *psHciContext,
                                void                    *pHwRef,
                                uint8_t                 reader_pipe_id
                             )
{
    NFCSTATUS                   status = NFCSTATUS_SUCCESS;

    if( ( NULL == psHciContext )
        || ( NULL == pHwRef )
        || ( HCI_UNKNOWN_PIPE_ID == reader_pipe_id)
        )
    {
        status = PHNFCSTVAL(CID_NFC_HCI, NFCSTATUS_INVALID_PARAMETER);
    }
    else
    {

        status = phHciNfc_Send_RFReader_Event ( psHciContext, pHwRef,
                    reader_pipe_id,(uint8_t) EVT_END_OPERATION );
        status = ( (status == NFCSTATUS_PENDING)?
                                NFCSTATUS_SUCCESS : status);
    }
    return status;
}


/*!
 * \brief Enable the Discovery of RF Reader Managment Gate.
 *
 * This function Enable the discovery of the RF Reader Management
 * gate.
 *
 */


NFCSTATUS
phHciNfc_ReaderMgmt_Enable_Discovery(
                                phHciNfc_sContext_t     *psHciContext,
                                void                    *pHwRef
                             )
{
    NFCSTATUS                   status = NFCSTATUS_SUCCESS;
    phHciNfc_ReaderMgmt_Info_t  *p_reader_mgmt_info=NULL;
    uint8_t                     reader_pipe_id = (uint8_t) HCI_UNKNOWN_PIPE_ID;
    /* phHal_sADD_Cfg_t    *p_poll_config = (phHal_sADD_Cfg_t * )
                                            psHciContext->p_config_params; */
    PHNFC_UNUSED_VARIABLE(p_reader_mgmt_info);
    if( NULL != psHciContext->p_reader_mgmt_info )
    {
        uint8_t rdr_enable = FALSE;
        p_reader_mgmt_info = (phHciNfc_ReaderMgmt_Info_t *)
                            psHciContext->p_reader_mgmt_info ;
#ifdef TYPE_B
        if ( (NULL != psHciContext->p_reader_b_info )
             /*   && (FALSE == rdr_enable) */
        )
        {
            /* Get the Reader B Pipe ID */
            status = phHciNfc_ReaderB_Get_PipeID
                                    (psHciContext, &reader_pipe_id);

            if( NFCSTATUS_SUCCESS == status )
            {
                rdr_enable = (uint8_t)TRUE;
                /* rdr_enable = (uint8_t)
                        p_poll_config->PollDevInfo.PollCfgInfo.EnableIso14443B; */
                status =    phHciNfc_ReaderB_Update_Info(psHciContext,
                                        HCI_RDR_ENABLE_TYPE, &rdr_enable);
            }
        }
#endif
#ifdef TYPE_FELICA
        if ( (NULL != psHciContext->p_felica_info )
               /* && (FALSE == rdr_enable) */
        )
        {
            /* Get the Reader F Pipe ID */
            status = phHciNfc_Felica_Get_PipeID
                                    (psHciContext, &reader_pipe_id);

            if( NFCSTATUS_SUCCESS == status )
            {
                rdr_enable = (uint8_t)TRUE;
               /* rdr_enable = (uint8_t)
                    ( p_poll_config->PollDevInfo.PollCfgInfo.EnableFelica212
                        || p_poll_config->PollDevInfo.PollCfgInfo.EnableFelica424 ); */
                status =    phHciNfc_Felica_Update_Info(psHciContext,
                                        HCI_RDR_ENABLE_TYPE, &rdr_enable);
            }
        }
#endif
#ifdef TYPE_JEWEL
        if ( (NULL != psHciContext->p_jewel_info )
               /* && (FALSE == rdr_enable) */
        )
        {
            /* Get the Reader F Pipe ID */
            status = phHciNfc_Jewel_Get_PipeID
                                    (psHciContext, &reader_pipe_id);

            if( NFCSTATUS_SUCCESS == status )
            {
                rdr_enable = (uint8_t)TRUE;
                status =    phHciNfc_Jewel_Update_Info(psHciContext,
                                        HCI_RDR_ENABLE_TYPE, &rdr_enable);
            }
        }
#endif /* #ifdef TYPE_JEWEL */
#if defined(TYPE_ISO15693)
        if ( (NULL != psHciContext->p_iso_15693_info )
               /* && (FALSE == rdr_enable) */
        )
        {
            /* Get the Reader F Pipe ID */
            status = phHciNfc_ISO15693_Get_PipeID
                                    (psHciContext, &reader_pipe_id);

            if( NFCSTATUS_SUCCESS == status )
            {
                rdr_enable = (uint8_t)TRUE;
                status =    phHciNfc_ISO15693_Update_Info(psHciContext,
                                        HCI_RDR_ENABLE_TYPE, &rdr_enable);
            }
        }

/* end of #elif defined(TYPE_ISO15693) */
#endif

        if(NULL != psHciContext->p_reader_a_info)
        {
            /* Get the Reader A Pipe ID */
            status = phHciNfc_ReaderA_Get_PipeID
                                    (psHciContext, &reader_pipe_id);

            if( NFCSTATUS_SUCCESS == status )
            {
                rdr_enable = (uint8_t)TRUE;
                status =    phHciNfc_ReaderA_Update_Info(psHciContext,
                                        HCI_RDR_ENABLE_TYPE, &rdr_enable);
            }

        }
        if( ( NFCSTATUS_SUCCESS == status )
            && (reader_pipe_id != HCI_UNKNOWN_PIPE_ID )
          )
        {
            status = phHciNfc_ReaderMgmt_Initiate_Discovery( psHciContext,
                         pHwRef, reader_pipe_id);
        }

    }/* End of the Reader Info Memory Check */

    return status;
}

/*!
 * \brief Disable the Discovery of RF Reader Managment Gate.
 *
 * This function Disable the discovery of the RF Reader Management
 * gate.
 *
 */

NFCSTATUS
phHciNfc_ReaderMgmt_Disable_Discovery(
                                phHciNfc_sContext_t     *psHciContext,
                                void                    *pHwRef
                             )
{
    NFCSTATUS                   status = NFCSTATUS_FAILED;
    phHciNfc_ReaderMgmt_Info_t  *p_reader_mgmt_info=NULL;
    uint8_t                     reader_pipe_id = (uint8_t) HCI_UNKNOWN_PIPE_ID;
    /* phHal_sADD_Cfg_t         *p_poll_config = (phHal_sADD_Cfg_t * )
                                            psHciContext->p_config_params; */
    PHNFC_UNUSED_VARIABLE(p_reader_mgmt_info);
    if( NULL != psHciContext->p_reader_mgmt_info )
    {
        p_reader_mgmt_info = (phHciNfc_ReaderMgmt_Info_t *)
                            psHciContext->p_reader_mgmt_info ;
        if(NULL != psHciContext->p_reader_a_info)
        {
            /* Get the Reader A Pipe ID */
            status = phHciNfc_ReaderA_Get_PipeID
                                    (psHciContext, &reader_pipe_id);
#if 0
            if( NFCSTATUS_SUCCESS == status )
            {
                uint8_t rdr_enable = (uint8_t) FALSE;
                status =    phHciNfc_ReaderA_Update_Info(psHciContext,
                                        HCI_RDR_ENABLE_TYPE, &rdr_enable);
            }
#endif

        }
#ifdef TYPE_B
        else if((NULL != psHciContext->p_reader_b_info )
            /* && (NFCSTATUS_SUCCESS != status) */
            )
        {
            /* Get the Reader B Pipe ID */
            status = phHciNfc_ReaderB_Get_PipeID
                                    (psHciContext, &reader_pipe_id);
        }
#endif
#ifdef TYPE_FELICA
        else if((NULL != psHciContext->p_felica_info )
            /* && (NFCSTATUS_SUCCESS != status) */
            )
        {
            /* Get the Reader B Pipe ID */
            status = phHciNfc_Felica_Get_PipeID
                                    (psHciContext, &reader_pipe_id);
        }
#endif
#ifdef TYPE_JEWEL
        else if((NULL != psHciContext->p_jewel_info )
            /* && (NFCSTATUS_SUCCESS != status) */
            )
        {
            /* Get the Reader B Pipe ID */
            status = phHciNfc_Jewel_Get_PipeID
                                    (psHciContext, &reader_pipe_id);
        }
#endif /* #ifdef TYPE_JEWEL */
#ifdef  TYPE_ISO15693
        else if((NULL != psHciContext->p_iso_15693_info )
            /* && (NFCSTATUS_SUCCESS != status) */
            )
        {
            /* Get the Reader B Pipe ID */
            status = phHciNfc_ISO15693_Get_PipeID
                                    (psHciContext, &reader_pipe_id);
        }
#endif /* #ifdef    TYPE_ISO15693 */

        else
        {
            status = PHNFCSTVAL(CID_NFC_HCI, NFCSTATUS_NOT_ALLOWED);
        }

        if( (NFCSTATUS_SUCCESS == status)
            && (reader_pipe_id != HCI_UNKNOWN_PIPE_ID )
            )
        {
            status = phHciNfc_ReaderMgmt_End_Discovery( psHciContext,
                         pHwRef, reader_pipe_id);
        }

    }/* End of the Reader Info Memory Check */

    return status;
}




/*!
* \brief Updates the Sequence of RF Reader Managment Gate.
*
* This function Resets/Updates the sequence of the RF Reader Management
* gate.
*
*/

NFCSTATUS
phHciNfc_ReaderMgmt_Info_Sequence(
                                   phHciNfc_sContext_t      *psHciContext,
                                   void                     *pHwRef
                               )
{
#if defined(NXP_NFCIP_ACTIVATE_DELAY)
    static uint8_t              nfc_atr_retry = 0;
#endif
    NFCSTATUS                   status = NFCSTATUS_SUCCESS;
    phHciNfc_ReaderMgmt_Info_t  *p_reader_mgmt_info=NULL;
#if 0
    uint8_t                     reader_pipe_id = (uint8_t) HCI_UNKNOWN_PIPE_ID;
    phHal_eRemDevType_t         target_type = phHal_eUnknown_DevType;
#endif


    PHNFC_UNUSED_VARIABLE(p_reader_mgmt_info);
    if( NULL == psHciContext )
    {
        status = PHNFCSTVAL(CID_NFC_HCI, NFCSTATUS_INVALID_PARAMETER);
    }
    else
    {
        if( NULL != psHciContext->p_reader_mgmt_info )
        {
            p_reader_mgmt_info = (phHciNfc_ReaderMgmt_Info_t *)
                psHciContext->p_reader_mgmt_info ;
            switch( psHciContext->host_rf_type )
            {

                case phHal_eISO14443_A_PCD:
                {
                    /* If the Target Info is updated then the Target
                     * is connected.
                     */
                    if(NULL == psHciContext->p_target_info)
                    {
#if defined(NXP_NFCIP_ACTIVATE_DELAY)
                        nfc_atr_retry = 0;
#endif
                        status = phHciNfc_ReaderA_Info_Sequence(
                                                    psHciContext, pHwRef );
                    }
                    else
                    {
                            status = phHciNfc_ReaderA_App_Data(
                                                psHciContext, pHwRef );
                            status = ((NFCSTATUS_PENDING == status )?
                                        NFCSTATUS_SUCCESS : status);
                    }
                    break;
                }
#ifdef ENABLE_P2P
                case phHal_eNfcIP1_Initiator:
                {
                    /* If the Target Info is updated then the Target
                     * is connected.
                     */
#ifdef NFCIP_CHECK
                    if(NULL == psHciContext->p_target_info)
#endif
                    {
                        status = phHciNfc_NfcIP_Info_Sequence(
                                  psHciContext, pHwRef
#ifdef NOTIFY_REQD
                                    ,(NULL == psHciContext->p_target_info)
#endif /* #ifdef NOTIFY_REQD */
                                           );
                    }
#ifdef NFCIP_CHECK
                    else
                    {
                        status = phHciNfc_NfcIP_GetATRInfo(
                                            psHciContext, pHwRef, NFCIP_INITIATOR );
#if defined(NXP_NFCIP_ACTIVATE_DELAY)
                        if (
                            (NFCSTATUS_PENDING == status)
                            && ( NFCIP_ACTIVATE_DELAY <= nfc_atr_retry)
                            )
                        {
                            nfc_atr_retry = 0;
                            status = NFCSTATUS_SUCCESS;
                        }
                        else
                        {
                            nfc_atr_retry++;
                        }
#else
                        status = ((NFCSTATUS_PENDING == status )?
                                        NFCSTATUS_SUCCESS : status);
#endif
                    }
#endif
                    break;
                }
#endif
#ifdef TYPE_B
                case phHal_eISO14443_B_PCD:
                {
                    if(NULL == psHciContext->p_target_info)
                    {
                        status = phHciNfc_ReaderB_Info_Sequence(
                                                    psHciContext, pHwRef );
                    }
                    break;
                }
#endif /* #ifdef TYPE_B */
#ifdef TYPE_FELICA
                case phHal_eFelica_PCD:
                {
                    if(NULL == psHciContext->p_target_info)
                    {
#if defined(NXP_NFCIP_ACTIVATE_DELAY)
                        nfc_atr_retry = 0;
#endif
                        status = phHciNfc_Felica_Info_Sequence(
                                                    psHciContext, pHwRef );
                    }
                    break;
                }
#endif /* #ifdef TYPE_FELICA */
#ifdef TYPE_JEWEL
                case phHal_eJewel_PCD:
                {
                    if(NULL == psHciContext->p_target_info)
                    {
                        status = phHciNfc_Jewel_Info_Sequence(
                                                    psHciContext, pHwRef );
                    }
                    break;
                }
#endif /* #ifdef TYPE_JEWEL */
#if defined(TYPE_ISO15693)
                case phHal_eISO15693_PCD:
                {
                    if(NULL == psHciContext->p_target_info)
                    {
                        status = phHciNfc_ISO15693_Info_Sequence(
                                                    psHciContext, pHwRef );
                    }
                    break;
                }
#endif
                default:
                {
                    break;
                }
            }

        }/* End of the Reader Info Memory Check */

    } /* End of Null Context Check */

    return status;

}


/*!
 * \brief Connects the the selected tag via RF Reader Gates.
 *
 * This function connects the selected tags via RF Reader Gate.
 * This function uses the RF Reader gate based on the type of the
 * tag specified.
 */


NFCSTATUS
phHciNfc_ReaderMgmt_Select(
                                    phHciNfc_sContext_t     *psHciContext,
                                    void                    *pHwRef,
                                    phHal_eRemDevType_t     target_type
                )
{
    NFCSTATUS                   status = NFCSTATUS_SUCCESS;
    uint8_t                     reader_pipe_id = (uint8_t) HCI_UNKNOWN_PIPE_ID;

    if( (NULL == psHciContext) || (NULL == pHwRef) )
    {
      status = PHNFCSTVAL(CID_NFC_HCI, NFCSTATUS_INVALID_PARAMETER);
    }
    else
    {
        switch (target_type)
        {
            case phHal_eMifare_PICC:
            case phHal_eISO14443_3A_PICC:
            {
                /* Get the Reader A Pipe ID */
                status = phHciNfc_ReaderA_Get_PipeID
                                        (psHciContext, &reader_pipe_id);

                if( (NFCSTATUS_SUCCESS == status)
                    && (reader_pipe_id != HCI_UNKNOWN_PIPE_ID )
                    )
                {
                    if (TRUE == psHciContext->p_target_info->ConnectWithPresence)
                    {
                       status = phHciNfc_ReaderMgmt_Reactivate(
                                  psHciContext, pHwRef, target_type );
                    }
                    else
                    {
                       status = phHciNfc_ReaderA_Get_DataRateMax(
                                        psHciContext, pHwRef );
                    }
                }
                break;
            }
            case phHal_eISO14443_A_PICC:
            case phHal_eISO14443_4A_PICC:
            {
#ifdef ENABLE_AUTO_ACTIVATE
                /* Get the Reader A Pipe ID */
                status = phHciNfc_ReaderA_Get_PipeID
                                        (psHciContext, &reader_pipe_id);

                if( (NFCSTATUS_SUCCESS == status)
                    && (reader_pipe_id != HCI_UNKNOWN_PIPE_ID )
                    )
                {
                    status = phHciNfc_Send_RFReader_Command (psHciContext,
                                    pHwRef, reader_pipe_id, NXP_WR_PRESCHECK );
                }
#else
                 if( NXP_NFC_DISCOVERY_SPM
                         == psHciContext->p_hw_ref->discovery_mode )
                 {
                       status = phHciNfc_ReaderA_Get_DataRateMax(
                                        psHciContext, pHwRef );
                 }
                 else
                 {
                    status = phHciNfc_ReaderA_Cont_Activate(
                                                    psHciContext, pHwRef);
                 }
#endif /* #ifdef ENABLE_AUTO_ACTIVATE */
                break;
            }
#ifdef TYPE_B
            case phHal_eISO14443_B_PICC:
            case phHal_eISO14443_4B_PICC:
            {
                /* Get the Reader B Pipe ID */
                status = phHciNfc_ReaderB_Get_PipeID
                                        (psHciContext, &reader_pipe_id);

                if( (NFCSTATUS_SUCCESS == status)
                    && (reader_pipe_id != HCI_UNKNOWN_PIPE_ID )
                    )
                {
                    if( NXP_NFC_DISCOVERY_SPM
                            == psHciContext->p_hw_ref->discovery_mode )
                    {
                        status = phHciNfc_ReaderA_Get_DataRateMax(
                                           psHciContext, pHwRef );
                    }
                    else if (TRUE == psHciContext->p_target_info->ConnectWithPresence)
                    {
                       status = phHciNfc_Send_RFReader_Command (psHciContext,
                                    pHwRef, reader_pipe_id, NXP_WR_PRESCHECK );
                    }
                    else
                    {
                       status = phHciNfc_ReaderA_Get_DataRateMax(
                                        psHciContext, pHwRef );
                    }
                    /* status = phHciNfc_ReaderMgmt_Reactivate(
                            psHciContext, pHwRef, target_type ); */
                }
                break;
            }
#ifdef NFC_TYPE_3B_RAW
            case phHal_eISO14443_3B_PICC:
            {
                status = phHciNfc_ReaderB_Get_PipeID
                                        (psHciContext, &reader_pipe_id);
                if( (NFCSTATUS_SUCCESS == status)
                    && (reader_pipe_id != HCI_UNKNOWN_PIPE_ID )
                    )
                {
                    if (TRUE == psHciContext->p_target_info->ConnectWithPresence)
                    {
                       status = phHciNfc_ReaderBRaw_Get_Pupi(psHciContext, pHwRef);
                    }
                }
                break;
            }
#endif
#endif /* #ifdef TYPE_B */
#ifdef TYPE_FELICA
            case phHal_eFelica_PICC:
            {
                status = phHciNfc_Felica_Get_PipeID
                                        (psHciContext, &reader_pipe_id);

                if( (NFCSTATUS_SUCCESS == status)
                    && (reader_pipe_id != HCI_UNKNOWN_PIPE_ID )
                    )
                {
                    /* Get the Reader Felica Pipe ID */
                    if (TRUE == psHciContext->p_target_info->ConnectWithPresence)
                    {
                       status = phHciNfc_ReaderMgmt_Reactivate(
                                     psHciContext, pHwRef, target_type );
                    }
                    else
                    {
                       status = phHciNfc_ReaderA_Get_DataRateMax(
                                        psHciContext, pHwRef );
                    }
                }
                break;
            }
#endif /* #ifdef TYPE_FELICA */
#ifdef TYPE_JEWEL
            case phHal_eJewel_PICC:
            {
                /* Get the Reader jewel Pipe ID */
                status = phHciNfc_Jewel_Get_PipeID
                                        (psHciContext, &reader_pipe_id);

                if( (NFCSTATUS_SUCCESS == status)
                    && (reader_pipe_id != HCI_UNKNOWN_PIPE_ID )
                    )
                {
                    if (TRUE == psHciContext->p_target_info->ConnectWithPresence)
                    {
                       status = phHciNfc_Jewel_GetRID(
                                        psHciContext, pHwRef);
                    }
                    else
                    {
                       status = phHciNfc_ReaderA_Get_DataRateMax(
                                        psHciContext, pHwRef );
                    }
                }
                break;
            }
#endif /* #ifdef TYPE_JEWEL */
#ifdef  TYPE_ISO15693
            case phHal_eISO15693_PICC:
            {
                /* Get the Reader ISO 15693 Pipe ID */
                status = phHciNfc_ISO15693_Get_PipeID
                                        (psHciContext, &reader_pipe_id);

                if( (NFCSTATUS_SUCCESS == status)
                    && (reader_pipe_id != HCI_UNKNOWN_PIPE_ID )
                    )
                {
                    if (TRUE == psHciContext->p_target_info->ConnectWithPresence)
                    {
                       /* TODO */
                       status = phHciNfc_ReaderA_Get_DataRateMax(
                                        psHciContext, pHwRef );
                    }
                    else
                    {
                       status = phHciNfc_ReaderA_Get_DataRateMax(
                                        psHciContext, pHwRef );
                    }
                }
                break;
            }
#endif /* #ifdef    TYPE_ISO15693 */
#ifdef ENABLE_P2P
            case phHal_eNfcIP1_Target:
            {
                if ( (phHal_eISO14443_A_PCD ==
                            psHciContext->host_rf_type )
                   || (phHal_eFelica_PCD ==
                            psHciContext->host_rf_type )
                   )
                {
                    status = phHciNfc_Initiator_Cont_Activate(
                                                    psHciContext, pHwRef);
                }
                else
                {
                    status = phHciNfc_NfcIP_Presence_Check (psHciContext, pHwRef);
                }
                break;
            }
#endif
            default:
            {
                status = PHNFCSTVAL(CID_NFC_HCI, NFCSTATUS_FEATURE_NOT_SUPPORTED);
                break;
            }

        } /* End of the tag_type Switch */
    }

    return status;
}

NFCSTATUS
phHciNfc_ReaderMgmt_UICC_Dispatch(
                                    phHciNfc_sContext_t     *psHciContext,
                                    void                    *pHwRef,
                                    phHal_eRemDevType_t     target_type
                )
{
    NFCSTATUS                   status = NFCSTATUS_SUCCESS;
    uint8_t                     reader_pipe_id = (uint8_t) HCI_UNKNOWN_PIPE_ID;
    phHciNfc_Pipe_Info_t        *p_pipe_info = NULL;


    if( (NULL == psHciContext) || (NULL == pHwRef) )
    {
      status = PHNFCSTVAL(CID_NFC_HCI, NFCSTATUS_INVALID_PARAMETER);
    }
    else if (NULL == psHciContext->p_target_info)
    {
        status = PHNFCSTVAL(CID_NFC_HCI, NFCSTATUS_INVALID_HCI_INFORMATION);
    }
    else
    {
        switch (target_type)
        {
            case phHal_eISO14443_A_PICC:
            case phHal_eISO14443_4A_PICC:
            {
                /* Get the Reader A Pipe ID */
                status = phHciNfc_ReaderA_Get_PipeID(
                                    psHciContext, &reader_pipe_id);
                p_pipe_info = psHciContext->p_pipe_list[reader_pipe_id];
                p_pipe_info->param_info = &psHciContext->p_target_info->
                                    RemoteDevInfo.Iso14443A_Info.Uid;
                p_pipe_info->param_length = psHciContext->p_target_info->
                                    RemoteDevInfo.Iso14443A_Info.UidLength;

                break;
            }
#ifdef TYPE_B
            case phHal_eISO14443_B_PICC:
            case phHal_eISO14443_4B_PICC:
            {
                /* Get the Reader B Pipe ID */
                status = phHciNfc_ReaderB_Get_PipeID
                    (psHciContext, &reader_pipe_id);

                p_pipe_info = psHciContext->p_pipe_list[reader_pipe_id];
                p_pipe_info->param_info = &psHciContext->p_target_info->
                        RemoteDevInfo.Iso14443B_Info.AtqB.AtqResInfo.Pupi;
                p_pipe_info->param_length = PHHAL_PUPI_LENGTH;
                break;
            }
#endif /* #ifdef TYPE_B */
            case phHal_eMifare_PICC:
            case phHal_eISO14443_3A_PICC:
            default:
            {
                status = PHNFCSTVAL(CID_NFC_HCI, NFCSTATUS_FEATURE_NOT_SUPPORTED);
                break;
            }

        } /* End of the tag_type Switch */
    }
    if( (NFCSTATUS_SUCCESS == status)
        && (reader_pipe_id != HCI_UNKNOWN_PIPE_ID )
        )
    {
        status = phHciNfc_Send_RFReader_Command (psHciContext,
                        pHwRef, reader_pipe_id, NXP_WR_DISPATCH_TO_UICC );
    }

    return status;
}


NFCSTATUS
phHciNfc_ReaderMgmt_Reactivate(
                                    phHciNfc_sContext_t     *psHciContext,
                                    void                    *pHwRef,
                                    phHal_eRemDevType_t     target_type
                )
{
    NFCSTATUS                   status = NFCSTATUS_SUCCESS;
    uint8_t                     reader_pipe_id = (uint8_t) HCI_UNKNOWN_PIPE_ID;
    phHciNfc_Pipe_Info_t        *p_pipe_info = NULL;


    if( (NULL == psHciContext) || (NULL == pHwRef) )
    {
      status = PHNFCSTVAL(CID_NFC_HCI, NFCSTATUS_INVALID_PARAMETER);
    }
    else if (NULL == psHciContext->p_target_info)
    {
        status = PHNFCSTVAL(CID_NFC_HCI, NFCSTATUS_INVALID_HCI_INFORMATION);
    }
    else
    {
        switch (target_type)
        {
            case phHal_eISO14443_A_PICC:
            case phHal_eMifare_PICC:
            case phHal_eISO14443_4A_PICC:
            case phHal_eISO14443_3A_PICC:
            {
                /* Get the Reader A Pipe ID */
                status = phHciNfc_ReaderA_Get_PipeID(
                                    psHciContext, &reader_pipe_id);
                p_pipe_info = psHciContext->p_pipe_list[reader_pipe_id];
                p_pipe_info->param_info = &psHciContext->p_target_info->
                                    RemoteDevInfo.Iso14443A_Info.Uid;
                p_pipe_info->param_length = psHciContext->p_target_info->
                                    RemoteDevInfo.Iso14443A_Info.UidLength;

                break;
            }
#ifdef TYPE_B
#ifdef NFC_TYPE_3B_RAW
            case phHal_eISO14443_3B_PICC:
            {
                status = phHciNfc_ReaderB_Get_PipeID(psHciContext, &reader_pipe_id);
                p_pipe_info = psHciContext->p_pipe_list[reader_pipe_id];
                status = phHciNfc_ReaderA_Get_DataRateMax(psHciContext, pHwRef );
                HCI_DEBUG("%x: Ignore Type B Activation \n",
                                                p_pipe_info->param_length);
                break;
            }
#endif
            case phHal_eISO14443_B_PICC:
            case phHal_eISO14443_4B_PICC:
            {

                /* Get the Reader B Pipe ID */
                status = phHciNfc_ReaderB_Get_PipeID
                    (psHciContext, &reader_pipe_id);
                p_pipe_info = psHciContext->p_pipe_list[reader_pipe_id];

#if defined(NFC_NFC_ISOB_IGNORE_REACTIVATION)
                status = phHciNfc_ReaderA_Get_DataRateMax(
                                        psHciContext, pHwRef );
                HCI_DEBUG("%x: Ignore Type B Activation \n",
                                                p_pipe_info->param_length);

#elif defined(NFC_NFC_ISOB_PRESENCE_FOR_REACTIVATION)

                HCI_DEBUG("%x: Replace Presence with the Reactivation \n",
                                                p_pipe_info->param_length);

                if( (NFCSTATUS_SUCCESS == status)
                    && (reader_pipe_id != HCI_UNKNOWN_PIPE_ID )
                    )
                {
                    status = phHciNfc_Send_RFReader_Command (psHciContext,
                            pHwRef, reader_pipe_id, NXP_WR_PRESCHECK );
                }

#elif !defined(NXP_NFC_ISOB_PUPI_RESET)&&( NXP_NFC_ISOB_PUPI_RESET != 0x01 )
                p_pipe_info->param_info = &psHciContext->p_target_info->
                        RemoteDevInfo.Iso14443B_Info.AtqB.AtqResInfo.Pupi;
                p_pipe_info->param_length = PHHAL_PUPI_LENGTH;
                HCI_DEBUG("%x: Activation With PUPI \n",
                                                p_pipe_info->param_length);
#else
                /* Ignore the PUPI for RE-Activation */
                HCI_DEBUG("%x: Activation Without PUPI \n",
                                                p_pipe_info->param_length);

#endif
                break;
            }
#endif /* #ifdef TYPE_B */
#ifdef TYPE_FELICA
            case phHal_eFelica_PICC:
            {
                /* Get the Felica Reader Pipe ID */
                status = phHciNfc_Felica_Get_PipeID
                                        (psHciContext, &reader_pipe_id);

                if( (NFCSTATUS_SUCCESS == status)
                    && (reader_pipe_id != HCI_UNKNOWN_PIPE_ID)
                    && (reader_pipe_id <= PHHCINFC_MAX_PIPE)
                    )
                {
                     p_pipe_info = psHciContext->p_pipe_list[reader_pipe_id];
                     p_pipe_info->param_info = &psHciContext->p_target_info->
                               RemoteDevInfo.Felica_Info.IDm;
                     p_pipe_info->param_length = PHHAL_FEL_ID_LEN;
                }
                break;
            }
#endif /* #ifdef TYPE_FELICA */
#ifdef ENABLE_P2P
            case phHal_eNfcIP1_Target:
            {
                /* Get the Initiator Pipe ID */
                status = phHciNfc_Initiator_Get_PipeID(
                                    psHciContext, &reader_pipe_id);
                p_pipe_info = psHciContext->p_pipe_list[reader_pipe_id];
                p_pipe_info->param_info = &psHciContext->p_target_info->
                                            RemoteDevInfo.NfcIP_Info.NFCID;
                p_pipe_info->param_length = psHciContext->p_target_info->
                                        RemoteDevInfo.NfcIP_Info.NFCID_Length;
                break;
            }
            case phHal_eNfcIP1_Initiator:
            {
                status = PHNFCSTVAL(CID_NFC_HCI, NFCSTATUS_FEATURE_NOT_SUPPORTED);
                break;
            }
#endif /* #ifdef ENABLE_P2P */
            default:
            {
                status = PHNFCSTVAL(CID_NFC_HCI, NFCSTATUS_FEATURE_NOT_SUPPORTED);
                break;
            }

        } /* End of the tag_type Switch */
    }
    if( (NFCSTATUS_SUCCESS == status)
        && (reader_pipe_id != HCI_UNKNOWN_PIPE_ID )
        )
    {
        status = phHciNfc_Send_RFReader_Command (psHciContext,
                        pHwRef, reader_pipe_id, NXP_WR_ACTIVATE_ID );
    }

    return status;
}


/*!
* \brief Activates the next Remote Target in the field.
*
* This function selects and activates the next tag present in the field.
*/


NFCSTATUS
phHciNfc_ReaderMgmt_Activate_Next(
                                  phHciNfc_sContext_t       *psHciContext,
                                  void                  *pHwRef
                                  )
{
    NFCSTATUS                   status = NFCSTATUS_SUCCESS;
    /* phHciNfc_Pipe_Info_t     *p_pipe_info = NULL; */
    uint8_t                     reader_pipe_id = (uint8_t) HCI_UNKNOWN_PIPE_ID;

    if( (NULL == psHciContext) || (NULL == pHwRef) )
    {
        status = PHNFCSTVAL(CID_NFC_HCI, NFCSTATUS_INVALID_PARAMETER);
    }
    else
    {
        switch ( psHciContext->host_rf_type )
        {
            case phHal_eISO14443_A_PCD:
            {
                /* Get the Reader A Pipe ID */
                status = phHciNfc_ReaderA_Get_PipeID
                    (psHciContext, &reader_pipe_id);

                break;
            }
#ifdef TYPE_B
            case phHal_eISO14443_B_PCD:
            {
                /* Get the Reader B Pipe ID */
                status = phHciNfc_ReaderB_Get_PipeID
                    (psHciContext, &reader_pipe_id);

                break;
            }
#endif /* #ifdef TYPE_B */
#ifdef TYPE_FELICA
            case phHal_eFelica_PCD:
            {
                /* Get the Felica Reader Pipe ID */
                status = phHciNfc_Felica_Get_PipeID
                                        (psHciContext, &reader_pipe_id);

                break;
            }
#endif /* #ifdef TYPE_FELICA */
#ifdef  TYPE_ISO15693
            case phHal_eISO15693_PCD:
            {
                /* Get the ISO 15693 Reader Pipe ID */
                status = phHciNfc_ISO15693_Get_PipeID
                                        (psHciContext, &reader_pipe_id);

                break;
            }
#endif /* #ifdef TYPE_ISO15693 */
            default:
            {
                status = PHNFCSTVAL(CID_NFC_HCI, NFCSTATUS_FEATURE_NOT_SUPPORTED);
                break;
            }

        } /* End of the reader_type Switch */
        if( (NFCSTATUS_SUCCESS == status)
            && (reader_pipe_id != HCI_UNKNOWN_PIPE_ID )
          )
        {
            status = phHciNfc_Send_RFReader_Command (psHciContext,
                pHwRef, reader_pipe_id, NXP_WR_ACTIVATE_NEXT );
        }

    }

    return status;

}

/*!
* \brief Checks the presence of the Remote Target in the field.
*
* This function checks the presence of the tag present in the field.
*/


NFCSTATUS
phHciNfc_ReaderMgmt_Presence_Check(
                                  phHciNfc_sContext_t       *psHciContext,
                                  void                  *pHwRef
                                  )
{
    NFCSTATUS                   status = NFCSTATUS_SUCCESS;
    /* phHciNfc_Pipe_Info_t     *p_pipe_info = NULL; */
    uint8_t                     reader_pipe_id = (uint8_t) HCI_UNKNOWN_PIPE_ID;
    phHal_eRemDevType_t         target_type = phHal_eUnknown_DevType;

    if( (NULL == psHciContext) || (NULL == pHwRef) )
    {
        status = PHNFCSTVAL(CID_NFC_HCI, NFCSTATUS_INVALID_PARAMETER);
    }
    else
    {
        target_type = psHciContext->p_target_info->RemDevType;
        switch (target_type)
        {
            case phHal_eMifare_PICC:
            case phHal_eISO14443_3A_PICC:
            {
                /* Get the Reader A Pipe ID */
                status = phHciNfc_ReaderA_Get_PipeID
                    (psHciContext, &reader_pipe_id);

                if( (NFCSTATUS_SUCCESS == status)
                    && (reader_pipe_id != HCI_UNKNOWN_PIPE_ID )
                    )
                {
                    status = phHciNfc_ReaderMgmt_Reactivate(
                            psHciContext, pHwRef, target_type );
                }
                break;
            }
            case phHal_eISO14443_A_PICC:
            case phHal_eISO14443_4A_PICC:
            {
                /* Get the Reader A Pipe ID */
                status = phHciNfc_ReaderA_Get_PipeID
                    (psHciContext, &reader_pipe_id);

                if( (NFCSTATUS_SUCCESS == status)
                    && (reader_pipe_id != HCI_UNKNOWN_PIPE_ID )
                    )
                {
                    status = phHciNfc_Send_RFReader_Command (psHciContext,
                        pHwRef, reader_pipe_id, NXP_WR_PRESCHECK );
                }
                break;
            }
#ifdef ENABLE_P2P
            case phHal_eNfcIP1_Target:
            {
                status = phHciNfc_NfcIP_Presence_Check (psHciContext, pHwRef);
                break;
            }
#endif
#ifdef TYPE_B
            case phHal_eISO14443_B_PICC:
            case phHal_eISO14443_4B_PICC:
            {
                /* Get the Reader B Pipe ID */
                status = phHciNfc_ReaderB_Get_PipeID
                    (psHciContext, &reader_pipe_id);

                if( (NFCSTATUS_SUCCESS == status)
                    && (reader_pipe_id != HCI_UNKNOWN_PIPE_ID )
                    )
                {
                    status = phHciNfc_Send_RFReader_Command (psHciContext,
                            pHwRef, reader_pipe_id, NXP_WR_PRESCHECK );
                }
                break;
            }
#ifdef NFC_TYPE_3B_RAW
            case phHal_eISO14443_3B_PICC:
            {
                status = phHciNfc_ReaderB_Get_PipeID
                         (psHciContext, &reader_pipe_id);
                if( (NFCSTATUS_SUCCESS == status)
                    && (reader_pipe_id != HCI_UNKNOWN_PIPE_ID )
                    )
                {
                    status = phHciNfc_ReaderBRaw_Get_Pupi(psHciContext,pHwRef);
                }
                break;
            }
#endif
#endif /* #ifdef TYPE_B */
#ifdef TYPE_FELICA
            case phHal_eFelica_PICC:
            {
                /* Get the Felica Reader Pipe ID */
                status = phHciNfc_Felica_Get_PipeID
                                        (psHciContext, &reader_pipe_id);

                if( (NFCSTATUS_SUCCESS == status)
                    && (reader_pipe_id != HCI_UNKNOWN_PIPE_ID )
                    )
                {
                    /* status = phHciNfc_Felica_Request_Mode(psHciContext, pHwRef); */
                    /* To check for the presence of the card */
                    status = phHciNfc_Felica_Poll_Request(psHciContext, pHwRef);
                }
                break;
            }
#endif /* #ifdef TYPE_FELICA */
#ifdef TYPE_JEWEL
            case phHal_eJewel_PICC:
            {
                /* Get the Jewel Reader Pipe ID */
                status = phHciNfc_Jewel_Get_PipeID
                                        (psHciContext, &reader_pipe_id);

                if( (NFCSTATUS_SUCCESS == status)
                    && (reader_pipe_id != HCI_UNKNOWN_PIPE_ID )
                    )
                {
                    /* status = PHNFCSTVAL(CID_NFC_HCI,
                                    NFCSTATUS_FEATURE_NOT_SUPPORTED); */
                      status = phHciNfc_Jewel_GetRID(
                                        psHciContext, pHwRef);
                }
                break;
            }
#endif /* #ifdef TYPE_JEWEL */
#ifdef  TYPE_ISO15693
            case phHal_eISO15693_PICC:
            {
                /* Get the Reader ISO 15693 Pipe ID */
                status = phHciNfc_ISO15693_Get_PipeID
                                        (psHciContext, &reader_pipe_id);

                if( (NFCSTATUS_SUCCESS == status)
                    && (reader_pipe_id != HCI_UNKNOWN_PIPE_ID )
                    )
                {

                    status = phHciNfc_ISO15693_Presence_Check(
                                       psHciContext,  pHwRef );

                }
                break;
            }
#endif /* #ifdef    TYPE_ISO15693 */
            default:
            {
                status = PHNFCSTVAL(CID_NFC_HCI,
                                    NFCSTATUS_FEATURE_NOT_SUPPORTED);
                break;
            }

        } /* End of the tag_type Switch */
    }

    return status;

}


/*!
 * \brief Disconnects the the selected tag.
 *
 * This function disconnects the selected tags via RF Reader Gate.
 * This function uses the RF Reader gate based on the type of the
 * tag specified.
 */


NFCSTATUS
phHciNfc_ReaderMgmt_Deselect(
                                    phHciNfc_sContext_t     *psHciContext,
                                    void                    *pHwRef,
                                    phHal_eRemDevType_t     target_type,
                                    uint8_t                 discovery_type
                )
{
    static  uint8_t             rls_param = NFC_RF_DISCOVERY_CONTINUE;
    NFCSTATUS                   status = NFCSTATUS_SUCCESS;
    uint8_t                     reader_pipe_id =
                                    (uint8_t) HCI_UNKNOWN_PIPE_ID;
    phHciNfc_Pipe_Info_t        *p_pipe_info = NULL;



    if( (NULL == psHciContext)
        || (NULL == pHwRef)
        )
    {
      status = PHNFCSTVAL(CID_NFC_HCI, NFCSTATUS_INVALID_PARAMETER);
    }
    else
    {
        rls_param = discovery_type;

        switch (target_type)
        {
            case phHal_eMifare_PICC:
            case phHal_eISO14443_A_PICC:
            case phHal_eISO14443_3A_PICC:
            case phHal_eISO14443_4A_PICC:
            {
                /* Get the Reader A Pipe ID */
                status = phHciNfc_ReaderA_Get_PipeID
                                        (psHciContext, &reader_pipe_id);
                break;
            }
#ifdef TYPE_B
            case phHal_eISO14443_B_PICC:
            case phHal_eISO14443_4B_PICC:
#ifdef NFC_TYPE_3B_RAW
            case phHal_eISO14443_3B_PICC:
#endif
            {
                /* Get the Reader B Pipe ID */
                status = phHciNfc_ReaderB_Get_PipeID
                    (psHciContext, &reader_pipe_id);

                break;
            }
#endif /* #ifdef TYPE_B */
#ifdef TYPE_FELICA
            case phHal_eFelica_PICC:
            {
                /* Get the Felica Pipe ID */
                status = phHciNfc_Felica_Get_PipeID
                    (psHciContext, &reader_pipe_id);

                break;
            }
#endif /* #ifdef TYPE_FELICA */
#ifdef TYPE_JEWEL
            case phHal_eJewel_PICC:
            {
                /* Get the Jewel Pipe ID */
                status = phHciNfc_Jewel_Get_PipeID
                    (psHciContext, &reader_pipe_id);

                break;
            }
#endif /* #ifdef TYPE_JEWEL */
#ifdef  TYPE_ISO15693
            case phHal_eISO15693_PICC:
            {
                /* Get the ISO 15693 Pipe ID */
                status = phHciNfc_ISO15693_Get_PipeID
                    (psHciContext, &reader_pipe_id);

                break;
            }
#endif /* #ifdef    TYPE_ISO15693 */
#ifdef ENABLE_P2P
            case phHal_eNfcIP1_Target:
            {
                /* Get the Reader A Pipe ID */
                status = phHciNfc_Initiator_Get_PipeID
                                        (psHciContext, &reader_pipe_id);

                break;
            }
#endif
            default:
            {
                status = PHNFCSTVAL(CID_NFC_HCI,
                                            NFCSTATUS_FEATURE_NOT_SUPPORTED);
                break;
            }

        } /* End of the tag_type Switch */
        p_pipe_info = psHciContext->p_pipe_list[reader_pipe_id];
        if( (NFCSTATUS_SUCCESS == status)
            && (reader_pipe_id != HCI_UNKNOWN_PIPE_ID )
            && ( NULL != p_pipe_info ) )
        {
            uint8_t     rf_discovery_event = NXP_EVT_RELEASE_TARGET;
            switch(rls_param)
            {
                case NFC_RF_DISCOVERY_REPOLL:
                {
                    p_pipe_info->param_info = &rls_param;
                    p_pipe_info->param_length = sizeof(rls_param);
                    break;
                }
                case NFC_RF_DISCOVERY_HALT:
                {
                    rf_discovery_event = EVT_END_OPERATION;
                    break;
                }
                case NFC_RF_DISCOVERY_CONTINUE:
                default:
                {
                    break;
                }
            }

            status = phHciNfc_Send_RFReader_Event ( psHciContext, pHwRef,
                        reader_pipe_id,(uint8_t) rf_discovery_event );

            if ( ( NFCSTATUS_PENDING ==  status )
                      && ( NXP_NFC_DISCOVERY_SPM
                               == psHciContext->p_hw_ref->discovery_mode )
               )
            {
                HCI_DEBUG("\t Target removal event awaited - %u\n", rf_discovery_event);
                psHciContext->event_pending = TRUE;
            }
        }
    }

    return status;
}


/*!
 * \brief Exchanges the data to/from the selected tags via RF Reader Gates.
 *
 * This function Exchanges the data to/from the selected tags
 * via RF Reader Gates. This function uses the RF Reader gate based on the
 * type of the selected tag and the type of the Reader gate specified.
 */


NFCSTATUS
phHciNfc_ReaderMgmt_Exchange_Data(
                                    phHciNfc_sContext_t     *psHciContext,
                                    void                    *pHwRef,
                                    phHciNfc_XchgInfo_t     *p_xchg_info
                )
{
    NFCSTATUS                   status = NFCSTATUS_SUCCESS;
    phHciNfc_Pipe_Info_t        *p_pipe_info = NULL;
    uint8_t                     reader_pipe_id = (uint8_t) HCI_UNKNOWN_PIPE_ID;
    phHal_eRemDevType_t         target_type = phHal_eUnknown_DevType;

    if( (NULL == psHciContext) || (NULL == pHwRef) )
    {
      status = PHNFCSTVAL(CID_NFC_HCI, NFCSTATUS_INVALID_PARAMETER);
    }
    else if (NULL == psHciContext->p_xchg_info)
    {
        status = PHNFCSTVAL(CID_NFC_HCI,
                NFCSTATUS_INVALID_HCI_INFORMATION);

    }
    else
    {
        switch (psHciContext->host_rf_type)
        {
            case phHal_eISO14443_A_PCD:
            {
                /* Get the Reader A Pipe ID */
                status = phHciNfc_ReaderA_Get_PipeID
                                        (psHciContext, &reader_pipe_id);

                if( (NFCSTATUS_SUCCESS == status)
                    && (reader_pipe_id != HCI_UNKNOWN_PIPE_ID)
                    && (reader_pipe_id <= PHHCINFC_MAX_PIPE)
                    )
                {
                    p_pipe_info = psHciContext->p_pipe_list[reader_pipe_id];
                    p_pipe_info->param_info = p_xchg_info->tx_buffer;
                    p_pipe_info->param_length = p_xchg_info->tx_length;
                    target_type = psHciContext->p_target_info->RemDevType;
                    switch (target_type)
                    {
                        case phHal_eMifare_PICC:
                        case phHal_eISO14443_3A_PICC:
                        {
                            if ((uint8_t)phHal_eMifareRaw ==
                                            p_xchg_info->params.tag_info.cmd_type)
                            {
                                status = phHciNfc_Send_ReaderA_Command(
                                    psHciContext,  pHwRef
                                    ,reader_pipe_id, NXP_MIFARE_RAW );
                            }
                            else
                            {
                                status = phHciNfc_Send_ReaderA_Command(
                                    psHciContext, pHwRef,
                                    reader_pipe_id, NXP_MIFARE_CMD );
                            }
                            break;
                        }
                        case phHal_eISO14443_A_PICC:
                        case phHal_eISO14443_4A_PICC:
                        {
                            if((NXP_NFC_ROM_INFO(((phHal_sHwReference_t *)pHwRef)->device_info.fw_version)
                               >= NFC44_ROM33_VER ))
                            {
                                status = phHciNfc_Send_RFReader_Command(
                                        psHciContext, pHwRef,
                                        reader_pipe_id,
                                        (p_xchg_info->timeout >> BYTE_SIZE )?
                                                   NXP_WR_XCHG_DATA_EXT: WR_XCHGDATA );
                            }
                            else
                            {
                                status = phHciNfc_Send_RFReader_Command(
                                        psHciContext, pHwRef,
                                        reader_pipe_id,
                                        WR_XCHGDATA
                                        );
                            }

                            break;
                        }
                        default:
                        {
                            status = PHNFCSTVAL(CID_NFC_HCI,
                                                NFCSTATUS_FEATURE_NOT_SUPPORTED);
                            break;
                        }
                    } /* End of the tag_type Switch */
                } /* End of Pipe ID Check */
                break;
            }
#ifdef TYPE_B
            case phHal_eISO14443_B_PCD:
            {
                /* Get the Reader B Pipe ID */
                status = phHciNfc_ReaderB_Get_PipeID
                                        (psHciContext, &reader_pipe_id);

                if( (NFCSTATUS_SUCCESS == status)
                    && (reader_pipe_id != HCI_UNKNOWN_PIPE_ID)
                    && (reader_pipe_id <= PHHCINFC_MAX_PIPE)
                    )
                {
                    p_pipe_info = psHciContext->p_pipe_list[reader_pipe_id];
                    p_pipe_info->param_info = p_xchg_info->tx_buffer;
                    p_pipe_info->param_length = p_xchg_info->tx_length;
#ifdef NFC_TYPE_3B_RAW
                    status = phHciNfc_Send_RFReader_Command(
                                      psHciContext, pHwRef,
                                      reader_pipe_id,
                                      (p_xchg_info->params.tag_info.cmd_type==(uint8_t)phHal_eIso14443_3B_Raw)?
                                      NXP_3B_RAW_XCH: WR_XCHGDATA);
#else
                    status = phHciNfc_Send_RFReader_Command(
                                psHciContext, pHwRef,
                                reader_pipe_id, WR_XCHGDATA );
#endif
                }
                break;
            }
#endif /* #ifdef TYPE_B */
#ifdef TYPE_FELICA
            case phHal_eFelica_PCD:
            {
                /* Get the Felica Reader Pipe ID */
                status = phHciNfc_Felica_Get_PipeID
                                        (psHciContext, &reader_pipe_id);

                if( (NFCSTATUS_SUCCESS == status)
                    && (reader_pipe_id != HCI_UNKNOWN_PIPE_ID)
                    && (reader_pipe_id <= PHHCINFC_MAX_PIPE)
                    )
                {
                    p_pipe_info = psHciContext->p_pipe_list[reader_pipe_id];
                    p_pipe_info->param_info = p_xchg_info->tx_buffer;
                    p_pipe_info->param_length = p_xchg_info->tx_length;
                    if ((uint8_t)phHal_eFelica_Raw ==
                                    p_xchg_info->params.tag_info.cmd_type)
                    {
                        status = phHciNfc_Send_Felica_Command(
                            psHciContext,  pHwRef
                            ,reader_pipe_id, NXP_FELICA_RAW );
                    }
                    else
                    {
                        status = phHciNfc_Send_Felica_Command(
                            psHciContext, pHwRef,
                            reader_pipe_id, NXP_FELICA_CMD );
                    }
                }
                break;
            }
#endif /* #ifdef TYPE_FELICA */
#if defined(TYPE_ISO15693)
            case phHal_eISO15693_PCD:
            {
                /* Get the ISO15693 Reader Pipe ID */
                status = phHciNfc_ISO15693_Get_PipeID
                                        (psHciContext, &reader_pipe_id);

                if( (NFCSTATUS_SUCCESS == status)
                    && (reader_pipe_id != HCI_UNKNOWN_PIPE_ID)
                    && (reader_pipe_id <= PHHCINFC_MAX_PIPE)
                    )
                {
                    p_pipe_info = psHciContext->p_pipe_list[reader_pipe_id];
                    p_pipe_info->param_info = p_xchg_info->tx_buffer;
                    p_pipe_info->param_length = p_xchg_info->tx_length;
                    if (((uint8_t)phHal_eIso15693_Cmd  ==
                                    p_xchg_info->params.tag_info.cmd_type)
#if defined(SUPPORT_ISO15693_RAW)
                        || ((uint8_t) phHal_eIso15693_Raw ==
                                    p_xchg_info->params.tag_info.cmd_type)
#endif
                     )
                    {
                        status = phHciNfc_Send_ISO15693_Command(
                            psHciContext,  pHwRef
                            ,reader_pipe_id, NXP_ISO15693_CMD );
                    }
                    else
                    {
                        status = PHNFCSTVAL(CID_NFC_HCI,
                                    NFCSTATUS_INVALID_PARAMETER);
                    }
                }
                break;
            }
#endif
#ifdef TYPE_JEWEL
            case phHal_eJewel_PCD:
            {
                /* Get the Jewel Reader Pipe ID */
                status = phHciNfc_Jewel_Get_PipeID
                                        (psHciContext, &reader_pipe_id);

                if( (NFCSTATUS_SUCCESS == status)
                    && (reader_pipe_id != HCI_UNKNOWN_PIPE_ID)
                    && (reader_pipe_id <= PHHCINFC_MAX_PIPE)
                    )
                {
                    uint8_t         transact_type = 0;
                    p_pipe_info = psHciContext->p_pipe_list[reader_pipe_id];
                    p_pipe_info->param_info = p_xchg_info->tx_buffer;
                    p_pipe_info->param_length = p_xchg_info->tx_length;
                    switch(p_xchg_info->params.tag_info.cmd_type)
                    {
                        case phHal_eJewel_Raw:
                        {
                            transact_type = NXP_JEWEL_RAW;
                            break;
                        }
                        case phHal_eJewel_Invalid:
                        default:
                        {
                            status = PHNFCSTVAL(CID_NFC_HCI,
                                                NFCSTATUS_INVALID_PARAMETER);
                            break;
                        }
                    }
                    if(0 != transact_type)
                    {
                        status = phHciNfc_Send_Jewel_Command(
                                    psHciContext,  pHwRef,
                                    reader_pipe_id, transact_type );
                    }
                }
                break;
            }
#endif /* #ifdef TYPE_JEWEL */
            default:
            {
                status = PHNFCSTVAL(CID_NFC_HCI,
                                                NFCSTATUS_FEATURE_NOT_SUPPORTED);
                break;
            }
        }/* End of Reader Type Switch */
    }

    return status;
}

#if ( ES_HW_VER >= 33 )

NFCSTATUS
phHciNfc_ReaderMgmt_Send (
                        phHciNfc_sContext_t             *psHciContext,
                        void                            *pHwRef,
                        phHal_sRemoteDevInformation_t   *p_remote_dev_info,
                        phHciNfc_XchgInfo_t             *p_send_param
                        )
{
    NFCSTATUS                       status = NFCSTATUS_SUCCESS;

    PHNFC_UNUSED_VARIABLE(p_remote_dev_info);

    psHciContext->p_xchg_info = p_send_param;

    if (0 != p_send_param->timeout)
    {
        p_send_param->timeout = (p_send_param->timeout >> BYTE_SIZE);
    }
    else
    {
        p_send_param->timeout = NFC_HCI_EXTND_ISO_MULTIPLIER;
    }

    switch (psHciContext->host_rf_type)
    {
        case phHal_eISO14443_A_PCD:
        {
            status = phHciNfc_ReaderA_Send( psHciContext,
                           pHwRef,  p_send_param );
            break;
        }
        case phHal_eISO14443_B_PCD:
        {
            status = phHciNfc_ReaderB_Send( psHciContext,
                           pHwRef,  p_send_param );
            break;
        }
        default:
        {
            status = PHNFCSTVAL(CID_NFC_HCI, NFCSTATUS_INVALID_PARAMETER);
            break;
        }
    }

    return status;
}

#endif /* ( ES_HW_VER >= 33 ) */


/*!
 * \brief Releases the resources allocated the RF Reader Management.
 *
 * This function Releases the resources allocated the RF Reader Management.
 */

NFCSTATUS
phHciNfc_ReaderMgmt_Release(
                                phHciNfc_sContext_t     *psHciContext,
                                void                    *pHwRef
                             )
{
    NFCSTATUS                           status = NFCSTATUS_SUCCESS;
    phHciNfc_Pipe_Info_t                *p_pipe_info = NULL;
    phHciNfc_ReaderMgmt_Info_t          *p_reader_mgmt_info=NULL;

    if( (NULL == psHciContext) || (NULL == pHwRef) )
    {
      status = PHNFCSTVAL(CID_NFC_HCI, NFCSTATUS_INVALID_PARAMETER);
    }
    else
    {
        if( NULL != psHciContext->p_reader_mgmt_info )
        {
            p_reader_mgmt_info = (phHciNfc_ReaderMgmt_Info_t *)
                                psHciContext->p_reader_mgmt_info ;
            switch(p_reader_mgmt_info->rf_gate_cur_seq)
            {
                /* Reader A pipe close sequence */
                case READERA_PIPE_CLOSE:
                {
                    p_pipe_info = ((phHciNfc_ReaderA_Info_t *)
                            psHciContext->p_reader_a_info)->p_pipe_info;

                    status = phHciNfc_Close_Pipe( psHciContext,
                                                    pHwRef, p_pipe_info );
                    if(status == NFCSTATUS_SUCCESS)
                    {
                        p_reader_mgmt_info->rf_gate_next_seq =
                                                    READERB_PIPE_CLOSE;
                        /* status = NFCSTATUS_PENDING; */
                    }
                    break;
                }
#ifdef TYPE_B
                /* Reader B pipe close sequence */
                case READERB_PIPE_CLOSE:
                {
                    p_pipe_info = ((phHciNfc_ReaderB_Info_t *)
                            psHciContext->p_reader_b_info)->p_pipe_info;

                    status = phHciNfc_Close_Pipe( psHciContext,
                                                        pHwRef, p_pipe_info );
                    if(status == NFCSTATUS_SUCCESS)
                    {
                        p_reader_mgmt_info->rf_gate_next_seq =
                                                        FELICA_PROP_PIPE_CLOSE;
                        status = NFCSTATUS_PENDING;
                    }
                    break;
                }
#endif /* #ifdef TYPE_B */
#ifdef TYPE_FELICA
                /* Felica Reader pipe close sequence */
                case FELICA_PROP_PIPE_CLOSE:
                {
                    p_pipe_info = ((phHciNfc_Felica_Info_t *)
                            psHciContext->p_felica_info)->p_pipe_info;

                    status = phHciNfc_Close_Pipe( psHciContext,
                                                        pHwRef, p_pipe_info );
                    if(status == NFCSTATUS_SUCCESS)
                    {
                        p_reader_mgmt_info->rf_gate_next_seq =
                                                NFCIP1_INITIATOR_PIPE_CLOSE;
                        /* status = NFCSTATUS_PENDING; */
                    }
                    break;
                }
#endif /* #ifdef TYPE_FELICA */
#ifdef ENABLE_P2P
                /* NFC-IP1 Initiator pipe Close sequence */
                case NFCIP1_INITIATOR_PIPE_CLOSE:
                {
                    p_pipe_info =
                        ((phHciNfc_NfcIP_Info_t *)psHciContext->
                                    p_nfcip_info)->p_init_pipe_info;
                    if(NULL == p_pipe_info )
                    {
                        status = PHNFCSTVAL(CID_NFC_HCI,
                                        NFCSTATUS_INVALID_HCI_SEQUENCE);
                    }
                    else
                    {
                        status = phHciNfc_Open_Pipe( psHciContext,
                                                        pHwRef, p_pipe_info );
                        if(status == NFCSTATUS_SUCCESS)
                        {
                            p_reader_mgmt_info->rf_gate_next_seq = READERA_PIPE_CLOSE;
                            status = NFCSTATUS_PENDING;
                        }
                    }
                    break;
                }
#endif /* #ifdef ENABLE_P2P */
                default:
                {
                    status = PHNFCSTVAL(CID_NFC_HCI, NFCSTATUS_INVALID_HCI_SEQUENCE);
                    break;
                }

            }/* End of the Sequence Switch */

        }/* End of the Reader Info Memory Check */

    } /* End of Null Context Check */

    return status;
}


/*!
 * \brief Sends the RF Reader HCI Events to the connected reader device.
 *
 * This function Sends the RF Reader HCI Event frames in the HCP packet format to the
 * connected reader device.
 */

 NFCSTATUS
 phHciNfc_Send_RFReader_Event (
                                phHciNfc_sContext_t *psHciContext,
                                void                *pHwRef,
                                uint8_t             pipe_id,
                                uint8_t             event
                    )
 {
    NFCSTATUS               status = NFCSTATUS_SUCCESS;
    phHciNfc_HCP_Packet_t   *hcp_packet = NULL;
    phHciNfc_HCP_Message_t  *hcp_message = NULL;
    phHciNfc_Pipe_Info_t    *p_pipe_info = NULL;
    uint16_t                 length = 0;
    uint8_t                 i = 0;

    if( (NULL == psHciContext)
        || ( pipe_id > PHHCINFC_MAX_PIPE)
        ||(NULL == psHciContext->p_pipe_list[pipe_id])
      )
    {
        status = PHNFCSTVAL(CID_NFC_HCI, NFCSTATUS_INVALID_PARAMETER);
        HCI_DEBUG("%s: Invalid Arguments passed \n",
                                                "phHciNfc_Send_RFReader_Event");
    }
    else
    {
        p_pipe_info = (phHciNfc_Pipe_Info_t *)
                                psHciContext->p_pipe_list[pipe_id];
        psHciContext->tx_total = 0 ;
        length +=  HCP_HEADER_LEN ;
        switch( event )
        {
            case EVT_READER_REQUESTED:
            case EVT_END_OPERATION:
            {

                hcp_packet = (phHciNfc_HCP_Packet_t *) psHciContext->send_buffer;
                /* Construct the HCP Frame */
                phHciNfc_Build_HCPFrame(hcp_packet,HCP_CHAINBIT_DEFAULT,
                                        (uint8_t) pipe_id, HCP_MSG_TYPE_EVENT, event);
                break;
            }
            case NXP_EVT_RELEASE_TARGET:
            {
                hcp_packet = (phHciNfc_HCP_Packet_t *) psHciContext->send_buffer;
                /* Construct the HCP Frame */
                phHciNfc_Build_HCPFrame(hcp_packet,HCP_CHAINBIT_DEFAULT,
                                        (uint8_t) pipe_id, HCP_MSG_TYPE_EVENT, event);
                hcp_message = &(hcp_packet->msg.message);
                phHciNfc_Append_HCPFrame((uint8_t *)hcp_message->payload,
                                            i, p_pipe_info->param_info,
                                            p_pipe_info->param_length);
                length =(uint16_t)(length + i + p_pipe_info->param_length);
                break;
            }
            default:
            {
                status = PHNFCSTVAL( CID_NFC_HCI, NFCSTATUS_FEATURE_NOT_SUPPORTED );
                HCI_DEBUG("%s: Statement Should Not Occur \n",
                                            "phHciNfc_Send_RFReader_Event");
                break;
            }
        }
        if( NFCSTATUS_SUCCESS == status )
        {
            p_pipe_info->sent_msg_type = HCP_MSG_TYPE_EVENT ;
            p_pipe_info->prev_msg = event ;
            psHciContext->tx_total = length;

            /* Send the Constructed HCP packet to the lower layer */
            status = phHciNfc_Send_HCP( psHciContext, pHwRef );
            p_pipe_info->prev_status = NFCSTATUS_PENDING;
        }
    }

    return status;
}

/*!
 * \brief Sends the RF Reader HCI Additonal Commands to the connected
 * reader device.
 *
 * This function Sends the RF Reader HCI Command frames in the HCP packet
 * format to the connected reader device.
 */

 NFCSTATUS
 phHciNfc_Send_RFReader_Command (
                                phHciNfc_sContext_t *psHciContext,
                                void                *pHwRef,
                                uint8_t             pipe_id,
                                uint8_t             cmd
                    )
 {
    phHciNfc_HCP_Packet_t   *hcp_packet = NULL;
    phHciNfc_HCP_Message_t  *hcp_message = NULL;
    phHciNfc_Pipe_Info_t    *p_pipe_info = NULL;
    uint16_t                i = 0;
    uint16_t                length=0;
    uint16_t                timeout = 0;
    NFCSTATUS               status = NFCSTATUS_SUCCESS;

    if( (NULL == psHciContext)
        || ( pipe_id > PHHCINFC_MAX_PIPE)
        ||(NULL == psHciContext->p_pipe_list[pipe_id])
      )
    {
        status = PHNFCSTVAL(CID_NFC_HCI, NFCSTATUS_INVALID_PARAMETER);
        HCI_DEBUG("%s: Invalid Arguments passed \n",
                                                "phHciNfc_Send_RFReader_Command");
    }
    else
    {
        p_pipe_info = (phHciNfc_Pipe_Info_t *)
                                psHciContext->p_pipe_list[pipe_id];
        psHciContext->tx_total = 0 ;
        length +=  HCP_HEADER_LEN ;


        switch( cmd )
        {
            case WR_XCHGDATA:
            {
                hcp_packet = (phHciNfc_HCP_Packet_t *) psHciContext->send_buffer;
                /* Construct the HCP Frame */
                phHciNfc_Build_HCPFrame(hcp_packet,HCP_CHAINBIT_DEFAULT,
                                        (uint8_t) pipe_id, HCP_MSG_TYPE_COMMAND, cmd);
                hcp_message = &(hcp_packet->msg.message);
                /* Frame Wait Timeout */
                if(psHciContext->p_xchg_info != NULL)
                {
                    uint32_t                temp;
                    uint8_t                 index;

                    timeout = psHciContext->p_xchg_info->timeout;
                    for(temp = (int)(timeout*NXP_NFC_TIMER_CONSTANT),index=0; (temp>>index) !=0; index++);
                    timeout = TIMEOUTACTIVATE(index);
                }
                else
                {
                    timeout = NXP_ISO_XCHG_TIMEOUT;
                }

                if((timeout < NXP_ISO_XCHG_TIMEOUT)
                   && (timeout != TIMEOUTACTIVATE(timeout))
                  )
                {
                    timeout = NXP_ISO_XCHG_TIMEOUT;
                }
                else if (timeout == TIMEOUTACTIVATE(timeout))
                {
                    timeout = 0x00;
                }

                hcp_message->payload[i++] = (uint8_t) timeout;

                phHciNfc_Append_HCPFrame((uint8_t *)hcp_message->payload,
                                            i, p_pipe_info->param_info,
                                            p_pipe_info->param_length);
                length =(uint16_t)(length + i + p_pipe_info->param_length);
                break;
            }
            case NXP_WR_XCHG_DATA_EXT:
            {
                hcp_packet = (phHciNfc_HCP_Packet_t *) psHciContext->send_buffer;
                /* Construct the HCP Frame */
                phHciNfc_Build_HCPFrame(hcp_packet,HCP_CHAINBIT_DEFAULT,
                                        (uint8_t) pipe_id, HCP_MSG_TYPE_COMMAND, cmd);
                hcp_message = &(hcp_packet->msg.message);
                /* Frame Wait Timeout */
                if(psHciContext->p_xchg_info != NULL)
                {
                    timeout = psHciContext->p_xchg_info->timeout;
                }

                if (0 != timeout)
                {
                    timeout = (timeout >> BYTE_SIZE);
                }
                else
                {
                    timeout = NFC_HCI_EXTND_ISO_MULTIPLIER;
                }
                hcp_message->payload[i++] = (uint8_t)timeout;

                phHciNfc_Append_HCPFrame((uint8_t *)hcp_message->payload,
                                            i, p_pipe_info->param_info,
                                            p_pipe_info->param_length);
                length =(uint16_t)(length + i + p_pipe_info->param_length);
                break;
            }
#ifdef NFC_TYPE_3B_RAW
            case NXP_3B_RAW_XCH:
            {
                hcp_packet = (phHciNfc_HCP_Packet_t *) psHciContext->send_buffer;
                phHciNfc_Build_HCPFrame(hcp_packet,HCP_CHAINBIT_DEFAULT,
                                        (uint8_t) pipe_id, HCP_MSG_TYPE_COMMAND, 0x12);
                hcp_message = &(hcp_packet->msg.message);
                hcp_message->payload[i++] = 0x02;
                hcp_message->payload[i++] = 0x00;
                phHciNfc_Append_HCPFrame((uint8_t *)hcp_message->payload,
                                            i, p_pipe_info->param_info,
                                            p_pipe_info->param_length);
                length =(uint16_t)(length + i + p_pipe_info->param_length);
                break;
            }
#endif
            case NXP_WR_PRESCHECK:
            case NXP_WR_ACTIVATE_NEXT:
            {

                hcp_packet = (phHciNfc_HCP_Packet_t *) psHciContext->send_buffer;
                /* Construct the HCP Frame */
                phHciNfc_Build_HCPFrame(hcp_packet,HCP_CHAINBIT_DEFAULT,
                                        (uint8_t) pipe_id, HCP_MSG_TYPE_COMMAND, cmd);
                break;
            }
            case NXP_WR_DISPATCH_TO_UICC:
            case NXP_WR_ACTIVATE_ID:
            {

                hcp_packet = (phHciNfc_HCP_Packet_t *) psHciContext->send_buffer;
                /* Construct the HCP Frame */
                phHciNfc_Build_HCPFrame(hcp_packet,HCP_CHAINBIT_DEFAULT,
                                        (uint8_t) pipe_id, HCP_MSG_TYPE_COMMAND, cmd);
                hcp_message = &(hcp_packet->msg.message);
                /* UID of the Card */
                phHciNfc_Append_HCPFrame((uint8_t *)hcp_message->payload,
                                            i, p_pipe_info->param_info,
                                            p_pipe_info->param_length);
                length =(uint16_t)(length + i + p_pipe_info->param_length);
                break;
            }
            default:
            {
                status = PHNFCSTVAL( CID_NFC_HCI, NFCSTATUS_FEATURE_NOT_SUPPORTED );
                HCI_DEBUG("%s: Statement Should Not Occur \n",
                                                "phHciNfc_Send_RFReader_Command");
                break;
            }
        }
        if( NFCSTATUS_SUCCESS == status )
        {
            p_pipe_info->sent_msg_type = HCP_MSG_TYPE_COMMAND;
            p_pipe_info->prev_msg = cmd;
            psHciContext->tx_total = length;
            psHciContext->response_pending = TRUE ;

            /* Send the Constructed HCP packet to the lower layer */
            status = phHciNfc_Send_HCP( psHciContext, pHwRef );
            p_pipe_info->prev_status = NFCSTATUS_PENDING;
        }
    }

    return status;
}

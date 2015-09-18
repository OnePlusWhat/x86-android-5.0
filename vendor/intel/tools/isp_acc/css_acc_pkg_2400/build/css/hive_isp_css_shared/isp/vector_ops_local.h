/*
 * INTEL CONFIDENTIAL
 *
 * Copyright (C) 2010 - 2013 Intel Corporation.
 * All Rights Reserved.
 *
 * The source code contained or described herein and all documents
 * related to the source code ("Material") are owned by Intel Corporation
 * or licensors. Title to the Material remains with Intel
 * Corporation or its licensors. The Material contains trade
 * secrets and proprietary and confidential information of Intel or its
 * licensors. The Material is protected by worldwide copyright
 * and trade secret laws and treaty provisions. No part of the Material may
 * be used, copied, reproduced, modified, published, uploaded, posted,
 * transmitted, distributed, or disclosed in any way without Intel's prior
 * express written permission.
 *
 * No License under any patent, copyright, trade secret or other intellectual
 * property right is granted to or conferred upon you by disclosure or
 * delivery of the Materials, either expressly, by implication, inducement,
 * estoppel or otherwise. Any license under such intellectual property rights
 * must be express and approved by Intel in writing.
 */

#ifndef __VECTOR_OPS_LOCAL_H_INCLUDED__
#define __VECTOR_OPS_LOCAL_H_INCLUDED__

typedef __register struct {
	tvector		v0;
	tvector		v1;
} tvector1x2;

/* data type for single precision, unsigned vectors */
typedef tvectoru tvector1wu;

/* data type for double precision, unsigned vectors */
typedef __register struct {
	tvector1wu	vec0;
	tvector1wu	vec1;
} tvector2wu;

#endif /* __VECTOR_OPS_LOCAL_H_INCLUDED__ */

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Misc/AssertionMacros.h"

DEFINE_LOG_CATEGORY_STATIC(LogAdaAssertions, Error, All);

#define A_ENSURE(InExpression) ensure(InExpression)
#define A_ENSURE_RET(InExpression, RetVal) \
	if (!ensure(InExpression)) \
	{ \
		return RetVal; \
	}

#define A_ENSURE_MSG(InExpression, InFormat, ... ) ensureMsgf(InExpression, InFormat, ##__VA_ARGS__)
#define A_ENSURE_MSG_RET(InExpression, RetVal, InFormat, ... ) \
	if (!ensureMsgf(InExpression, InFormat, ##__VA_ARGS__)) \
	{ \
		return RetVal; \
	}

#if !UE_BUILD_SHIPPING

#define A_VALIDATE_OBJ(Object, RetVal) \
	if (!::IsValid(Object)) \
	{ \
		UE_LOG(LogAdaAssertions, Error, TEXT("%hs: Invalid object %hs"), __FUNCTION__, #Object); \
		return RetVal; \
	}

#define A_VALIDATE_OBJ_CTOR(Object) \
	if (!::IsValid(Object)) \
	{ \
		UE_LOG(LogAdaAssertions, Error, TEXT("%hs: Invalid object %hs"), __FUNCTION__, #Object); \
		return; \
	}

#define A_VALIDATE_PTR(Object, RetVal) \
	if (!Object) \
	{ \
		UE_LOG(LogAdaAssertions, Error, TEXT("%hs: Invalid pointer %hs"), __FUNCTION__, #Object); \
		return RetVal; \
	}

#define A_VALIDATE_PTR_CTOR(Object) \
	if (!Object) \
	{ \
		UE_LOG(LogAdaAssertions, Error, TEXT("%hs: Invalid pointer %hs"), __FUNCTION__, #Object); \
		return; \
	}

#else

#define A_VALIDATE_OBJ(Object, RetVal) \
	if (!::IsValid(Object)) \
	{ \
		return RetVal; \
	}

#define A_VALIDATE_OBJ_CTOR(Object) \
	if (!::IsValid(Object)) \
	{ \
		return; \
	}

#define A_VALIDATE_PTR(Object, RetVal) \
	if (!Object) \
	{ \
		return RetVal; \
	}

#define A_VALIDATE_PTR_CTOR(Object) \
	if (!Object) \
	{ \
		return; \
	}

#endif
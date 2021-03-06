/*
 *  Copyright Beijing 58 Information Technology Co.,Ltd.
 *
 *  Licensed to the Apache Software Foundation (ASF) under one
 *  or more contributor license agreements.  See the NOTICE file
 *  distributed with this work for additional information
 *  regarding copyright ownership.  The ASF licenses this file
 *  to you under the Apache License, Version 2.0 (the
 *  "License"); you may not use this file except in compliance
 *  with the License.  You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing,
 *  software distributed under the License is distributed on an
 *  "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 *  KIND, either express or implied.  See the License for the
 *  specific language governing permissions and limitations
 *  under the License.
 */
#include "derializer.h"
#include "byteHelper.h"
#include "structHelper.h"
#include "strHelper.h"
#include "serializeList.h"
#include "../protocol/SdpStruct.h"
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <objc/hash.h>

void* GetRef(array *refArray, int hashcode) {
	int dataLen = refArray->byteLength / sizeof(void *);
	if (dataLen == 0 || hashcode - 1000 > dataLen) {
		printf("derialize error data GetRef------------------\n");
		return NULL;
	}
	int index = hashcode - 1001;
	return *(void**) (refArray->data + index * sizeof(void *));
}
void SetRef(array *refArray, int hashcode, void *obj) {
	int dataLen = refArray->byteLength / sizeof(void *);
	if (dataLen + 1001 == hashcode) {
		byteArrayPutData(refArray, &obj, sizeof(void*));
	} else {
		printf("DetRef error data ------------------hashcode=%d,refObjArray.size=%d\n", hashcode, refArray->byteLength);
	}
}
void setNullValue(int typeId, array*retObj) {
	int size = getObjectSize(typeId);
	switch (typeId) {
	case SERIALIZE_CHAR_N:
	case SERIALIZE_BOOL_N:
	case SERIALIZE_SHORT_INT_N:
	case SERIALIZE_INT_N:
	case SERIALIZE_TIME_N:
	case SERIALIZE_FLOAT_N:
	case SERIALIZE_DOUBLE_N:
	case SERIALIZE_LONG_LONG_N:
	case SERIALIZE_ARRAY_N:
	case SERIALIZE_LIST_N:
	case SERIALIZE_MAP_N:
	case SERIALIZE_VOID_N:
		memset(retObj->data + retObj->byteLength, 0, size);
		retObj->byteLength += size;
		break;
	case SERIALIZE_STRING_N:
	case SERIALIZE_ENUM_N:
		structPutData(retObj, '\0', 1);
		break;
	default:
		memset(retObj->data + retObj->byteLength, 0, size);
		retObj->byteLength += size;
		break;
	}
}
void* DerializeMalloc(int typeId, int number) {
	int size = getObjectSize(typeId);
	if (size > 0) {
		return malloc(size * number);
	}
	return NULL;
}

int nullDerializer(int typeId, char isPointe, array *retObj, array *inData, array *refObjArray) {
	memset(retObj->data + retObj->byteLength, 0, sizeof(void*));
	retObj->byteLength += sizeof(void*);
	return 0;
}
int enumDerializer(int typeId, char isPointe, array *retObj, array *inData, array *refObjArray) {
	char *refFlg = byteArrayPopData(inData, 1);
	int *hashcode = byteArrayPopData(inData, 4);
	if (*refFlg) {
		void *refObj = GetRef(refObjArray, *hashcode);
		if (refObj) {
			int dataLen = strlen(refObj);
			if (isPointe) {
				structPutData(retObj, &refObj, sizeof(void*));
			} else {
				structPutData(retObj, refObj, dataLen + 1);
			}
		} else {
			if (isPointe) {
				nullDerializer(typeId, isPointe, retObj, inData, refObjArray);
			} else {
				structPutData(retObj, &refObj, 1);
			}
		}
	} else {
		int *len = byteArrayPopData(inData, 4);
		if (*len == 0) {
			SetRef(refObjArray, *hashcode, NULL);
			if (isPointe) {
				nullDerializer(typeId, isPointe, retObj, inData, refObjArray);
			} else {
				structPutData(retObj, len, 1);
			}
		} else {
			char *data = byteArrayPopData(inData, *len);
			if (isPointe) {
				char *newData = malloc(*len + 1);
				SetRef(refObjArray, *hashcode, newData);
				memcpy(newData, data, *len);
				newData[*len] = '\0';
				structPutData(retObj, &newData, sizeof(void*));
			} else {
				if (retObj->byteLength > 0) {
					printf("derializer string error!!!!");
					return -1;
				}
				retObj->data = realloc(retObj->data, *len + 1);
				SetRef(refObjArray, *hashcode, retObj->data);
				structPutData(retObj, data, *len);
				char c = '\0';
				structPutData(retObj, &c, 1);
			}
		}
	}
	return 0;
}
int charDerializer(int typeId, char isPointe, array *retObj, array *inData, array *refObjArray) {
	if (isPointe) {
		char *data = malloc(1);
		structPutData(retObj, &data, sizeof(void*));
		data[0] = *(char*) byteArrayPopData(inData, 1);
	} else {
		structPutData(retObj, byteArrayPopData(inData, 1), 1);
	}
	return 0;
}
int stringDerializer(int typeId, char isPointe, array *retObj, array *inData, array *refObjArray) {
	char *refFlg = byteArrayPopData(inData, 1);
	int *hashcode = byteArrayPopData(inData, 4);
	if (*refFlg) {
		void *refObj = GetRef(refObjArray, *hashcode);
		if (refObj) {
			int dataLen = strlen(refObj);
			if (isPointe) {
				structPutData(retObj, &refObj, sizeof(void*));
			} else {
				structPutData(retObj, refObj, dataLen + 1);
			}
		} else {
			if (isPointe) {
				nullDerializer(typeId, isPointe, retObj, inData, refObjArray);
			} else {
				structPutData(retObj, &refObj, 1);
			}
		}
	} else {
		int *len = byteArrayPopData(inData, 4);
		if (*len == 0) {
			SetRef(refObjArray, *hashcode, NULL);
			if (isPointe) {
				nullDerializer(typeId, isPointe, retObj, inData, refObjArray);
			} else {
				structPutData(retObj, len, 1);
			}
		} else {
			char *data = byteArrayPopData(inData, *len);
			if (isPointe) {
				char *newData = malloc(*len + 1);
				SetRef(refObjArray, *hashcode, newData);
				memcpy(newData, data, *len);
				newData[*len] = '\0';
				structPutData(retObj, &newData, sizeof(void*));
			} else {
				if (retObj->byteLength > 0) {
					printf("derializer string error!!!!");
					return -1;
				}
				retObj->data = realloc(retObj->data, *len + 1);
				SetRef(refObjArray, *hashcode, retObj->data);
				structPutData(retObj, data, *len);
				char c = '\0';
				structPutData(retObj, &c, 1);
			}
		}
	}
	return 0;
}
int shortIntDerializer(int typeId, char isPointe, array *retObj, array *inData, array *refObjArray) {
	if (isPointe) {
		short *data = malloc(2);
		structPutData(retObj, &data, sizeof(void*));
		data[0] = *(short*) byteArrayPopData(inData, 2);
	} else {
		structPutData(retObj, byteArrayPopData(inData, 2), 2);
	}
	return 0;
}
int intDerializer(int typeId, char isPointe, array *retObj, array *inData, array *refObjArray) {
	if (isPointe) {
		int *data = malloc(4);
		structPutData(retObj, &data, sizeof(void*));
		data[0] = *(int*) byteArrayPopData(inData, 4);
	} else {
		structPutData(retObj, byteArrayPopData(inData, 4), 4);
	}
	return 0;
}
int floatDerializer(int typeId, char isPointe, array *retObj, array *inData, array *refObjArray) {
	if (isPointe) {
		float *data = malloc(4);
		structPutData(retObj, &data, sizeof(void*));
		data[0] = *(float*) byteArrayPopData(inData, 4);
	} else {
		structPutData(retObj, byteArrayPopData(inData, 4), 4);
	}
	return 0;
}
int doubleDerializer(int typeId, char isPointe, array *retObj, array *inData, array *refObjArray) {
	if (isPointe) {
		double *data = malloc(8);
		structPutData(retObj, &data, sizeof(void*));
		data[0] = *(double*) byteArrayPopData(inData, 8);
	} else {
		structPutData(retObj, byteArrayPopData(inData, 8), 8);
	}
	return 0;
}
int timeDerializer(int typeId, char isPointe, array *retObj, array *inData, array *refObjArray) {
	long long *t = byteArrayPopData(inData, 8);
	long long l = *t / 1000;
	l -= 8 * 60 * 60;
	if (isPointe) {
		time_t *data = malloc(sizeof(time_t));
		structPutData(retObj, &data, sizeof(void*));
		data[0] = l;
	} else {
		structPutData(retObj, &l, sizeof(time_t));
	}
	return 0;
}
int longLongDerializer(int typeId, char isPointe, array *retObj, array *inData, array *refObjArray) {
	if (isPointe) {
		long long *data = malloc(8);
		structPutData(retObj, &data, sizeof(void*));
		data[0] = *(long long *) byteArrayPopData(inData, 8);
	} else {
		structPutData(retObj, byteArrayPopData(inData, 8), 8);
	}
	return 0;
}
int arrayDerializer(int typeId, char isPointe, array *retObj, array *inData, array *refObjArray) {
	int *_typeId = byteArrayPopData(inData, 4);
	char *refFlg = byteArrayPopData(inData, 1);
	int *hashcode = byteArrayPopData(inData, 4);
	if (*refFlg) {
		array *refObj = GetRef(refObjArray, *hashcode);
		if (refObj) {
			if (isPointe) {
				structPutData(retObj, &refObj, sizeof(void*));
			} else {
				structPutData(retObj, refObj, sizeof(array));
			}
		} else {
			if (isPointe) {
				nullDerializer(typeId, isPointe, retObj, inData, refObjArray);
			} else {
				setNullValue(typeId, retObj);
			}
		}
	} else {
		array *tempArray;
		if (isPointe) {
			tempArray = malloc(sizeof(array));
			structPutData(retObj, &tempArray, sizeof(void*));
		} else {
			tempArray = retObj->data + retObj->byteLength;
			retObj->byteLength += sizeof(array);
		}
		int *dataLen = byteArrayPopData(inData, 4);
		int isP = IsPrimitive(*_typeId);
		tempArray->objectLength = *dataLen;
		tempArray->byteLength = 0;
		tempArray->isPointe = 1;
		tempArray->typeId = *_typeId;
		tempArray->data = malloc(sizeof(void*) * *dataLen);
		SetRef(refObjArray, *hashcode, tempArray);
		int i = 0;
		for (i = 0; i < *dataLen; ++i) {
			if (!isP) {
				_typeId = byteArrayPopData(inData, 4);
			}
			derializer dr = getDerializer(*_typeId);
			dr(*_typeId, tempArray->isPointe, tempArray, inData, refObjArray);
		}
	}
	return 0;
}
int listDerializer(int typeId, char isPointe, array *retObj, array *inData, array *refObjArray) {
	int *_typeId = byteArrayPopData(inData, 4);
	char *refFlg = byteArrayPopData(inData, 1);
	int *hashcode = byteArrayPopData(inData, 4);
	if (*refFlg) {
		struct serialize_list *refObj = GetRef(refObjArray, *hashcode);
		if (refObj) {
			if (isPointe) {
				structPutData(retObj, &refObj, sizeof(void*));
			} else {
				structPutData(retObj, refObj, sizeof(struct serialize_list));
			}
		} else {
			if (isPointe) {
				nullDerializer(typeId, isPointe, retObj, inData, refObjArray);
			} else {
				setNullValue(typeId, retObj);
			}
		}
	} else {
		struct serialize_list *tempList;
		int *dataLen = byteArrayPopData(inData, 4);
		int i = 0;
		array tempArray;
		tempArray.byteLength = 0;
		tempArray.isPointe = 0;
		if (isPointe) {
			tempList = malloc(sizeof(struct serialize_list));
			tempList->tail = NULL;
			structPutData(retObj, &tempList, sizeof(void*));
		} else {
			tempList = retObj->data + retObj->byteLength;
			tempList->tail = NULL;
			retObj->byteLength += sizeof(struct serialize_list);
		}
		SetRef(refObjArray, *hashcode, tempList);
		void *data;
		derializer dr;
		for (i = 0; i < *dataLen; ++i) {
			_typeId = byteArrayPopData(inData, 4);
			dr = getDerializer(*_typeId);
			data = DerializeMalloc(*_typeId, 1);
			tempArray.data = data;
			tempArray.byteLength = 0;
			dr(*_typeId, 0, &tempArray, inData, refObjArray);
			if (i == 0) {
				tempList->head = data;
				tempList->typeId = *_typeId;
			} else {
				tempList = list_cons_back(data, *_typeId, tempList);
			}
		}
	}
	return 0;
}
int mapDerializer(int typeId, char isPointe, array *retObj, array *inData, array *refObjArray) {
	int *_typeId = byteArrayPopData(inData, 4);
	char *refFlg = byteArrayPopData(inData, 1);
	int *hashcode = byteArrayPopData(inData, 4);
	if (*refFlg) {
		cache_ptr *refObj = GetRef(refObjArray, *hashcode);
		structPutData(retObj, &refObj, sizeof(void*));
	} else {
		cache_ptr hashmap = objc_hash_new((unsigned int) 128, objHashFuncType, objCompareFuncType);
		SetRef(refObjArray, *hashcode, &hashmap);
		int *dataLen = byteArrayPopData(inData, 4);
		int i = 0;
		array keyArray, valueArray;
		keyArray.byteLength = 0;
		keyArray.isPointe = 0;
		valueArray.byteLength = 0;
		valueArray.isPointe = 0;
		int *valueTypeId;
		hashmapEntry *keyNode, *valueNode;
		derializer dr;
		for (i = 0; i < *dataLen; ++i) {
			_typeId = byteArrayPopData(inData, 4);
			keyArray.data = DerializeMalloc(*_typeId, 1);
			keyArray.byteLength = 0;
			dr = getDerializer(*_typeId);
			dr(*_typeId, 0, &keyArray, inData, refObjArray);

			valueTypeId = byteArrayPopData(inData, 4);
			if (*valueTypeId == 0) {
				valueArray.data = NULL;
			} else {
				valueArray.data = DerializeMalloc(*valueTypeId, 1);
				valueArray.byteLength = 0;
				dr = getDerializer(*valueTypeId);
				dr(*valueTypeId, 0, &valueArray, inData, refObjArray);
			}
			keyNode = malloc(sizeof(hashmapEntry));
			valueNode = malloc(sizeof(hashmapEntry));
			keyNode->typeId = *_typeId;
			keyNode->data = keyArray.data;
			valueNode->typeId = *valueTypeId;
			valueNode->data = valueArray.data;
			objc_hash_add(&hashmap, keyNode, valueNode);
		}
		structPutData(retObj, &hashmap, sizeof(void*));

	}
	return 0;
}
int structDerializer(int typeId, char isPointe, array *retObj, array *inData, array *refObjArray) {
	int *_typeId = byteArrayPopData(inData, 4);
	if (*_typeId == 0) {
		if (isPointe) {
			nullDerializer(*_typeId, isPointe, retObj, NULL, refObjArray);
		} else {
			setNullValue(*_typeId, retObj);
		}
		return 0;
	} else if (*_typeId != typeId) {
		printf("struct derialize error ,hashcode=%d,src struct hashcode=%d\n", *_typeId, typeId);
		return -1;
	}
	array *structInfo = objc_hash_value_for_key(structInfoMap, _typeId);
	if (structInfo == NULL || structInfo->byteLength == 0) {
		printf("not find structInfo %d\n", *_typeId);
		return -1;
	}
	structFieldInfo *sfi = structInfo->data;

	char *refFlg;
	int *hashcode;

	if (sfi[1].typeId != SERIALIZE_ENUM_N) {
		refFlg = byteArrayPopData(inData, 1);
		hashcode = byteArrayPopData(inData, 4);
	} else {
		refFlg = NULL;
		hashcode = NULL;
	}
	int structSize = sfi->offset;
	if (refFlg && *refFlg) {
		void *refObj = GetRef(refObjArray, *hashcode);
		if (refObj) {
			if (isPointe) {
				structPutData(retObj, &refObj, sizeof(void*));
			} else {
				structPutData(retObj, refObj, structSize);
			}
		} else {
			if (isPointe) {
				nullDerializer(*_typeId, sfi->isPointe, retObj, NULL, refObjArray);
			} else {
				setNullValue(*_typeId, retObj);
			}
		}
	} else {
		int i = 0;
		int fieldLen = structInfo->byteLength / sizeof(structFieldInfo);
		array *tempArray, ta;
		if (isPointe) {
			tempArray = &ta;
			tempArray->byteLength = 0;
			tempArray->data = malloc(structSize);
			structPutData(retObj, &(tempArray->data), sizeof(void*));
		} else {
			tempArray = retObj;
		}
		if (sfi[1].typeId != SERIALIZE_ENUM_N) {
			SetRef(refObjArray, *hashcode, tempArray->data + tempArray->byteLength);
		}
		int offset = tempArray->byteLength;
		++sfi;
		for (i = 1; i < fieldLen; ++i) {
			tempArray->byteLength = offset + sfi->offset;
			if (sfi->typeId != SERIALIZE_ENUM_N) {
				_typeId = byteArrayPopData(inData, 4);
				if (*_typeId == 0) {
					if (sfi->isPointe) {
						nullDerializer(*_typeId, sfi->isPointe, tempArray, NULL, refObjArray);
					} else {
						setNullValue(*_typeId, tempArray);
					}
				} else {
					derializer dr = getDerializer(*_typeId);
					dr(*_typeId, sfi->isPointe, tempArray, inData, refObjArray);
					if (sfi->typeId == SERIALIZE_VOID_N) {
						int *t = (int*) (tempArray->data + tempArray->byteLength + sizeof(void*));
						t[0] = *_typeId;
					}
				}
			} else {
				derializer dr = getDerializer(sfi->typeId);
				dr(sfi->typeId, sfi->isPointe, tempArray, inData, refObjArray);
			}
			++sfi;
		}
		tempArray->byteLength = offset + structSize;
	}
	return 0;
}

derializer getDerializer(int typeId) {
	derializer sr;
	switch (typeId) {
	case SERIALIZE_NULL_N:
		sr = nullDerializer;
		break;
	case SERIALIZE_CHAR_N:
	case SERIALIZE_BOOL_N:
		sr = charDerializer;
		break;
	case SERIALIZE_SHORT_INT_N:
		sr = shortIntDerializer;
		break;
	case SERIALIZE_INT_N:
		sr = intDerializer;
		break;
	case SERIALIZE_TIME_N:
		sr = timeDerializer;
		break;
	case SERIALIZE_FLOAT_N:
		sr = floatDerializer;
		break;
	case SERIALIZE_DOUBLE_N:
		sr = doubleDerializer;
		break;
	case SERIALIZE_LONG_LONG_N:
		sr = longLongDerializer;
		break;
	case SERIALIZE_ARRAY_N:
		sr = arrayDerializer;
		break;
	case SERIALIZE_LIST_N:
		sr = listDerializer;
		break;
	case SERIALIZE_MAP_N:
		sr = mapDerializer;
		break;
	case SERIALIZE_STRING_N:
		sr = stringDerializer;
		break;
	case SERIALIZE_ENUM_N:
		sr = enumDerializer;
		break;
	default: {
		sr = structDerializer;
		break;
	}
	}

	return sr;
}
void* Derialize(char *type, void *inData, int inDataLen) {
	if (inData == NULL) {
		return NULL;
	}
	int typeId = GetTypeId(type);
	array refObjArray = { byteLength:0, data:NULL};
	derializer dr = getDerializer(typeId);
	array retObj;
	retObj.byteLength = 0;
	retObj.data = DerializeMalloc(typeId, 1);
	array inArray = { byteLength:inDataLen, data:inData };
	dr(typeId, 0, &retObj, &inArray, &refObjArray);
	if (refObjArray.data) {
		free(refObjArray.data);
	}
	return retObj.data;
}
static void DerializeFreeN(int typeId, void *obj) {

	switch (typeId) {
	case SERIALIZE_CHAR_N:
	case SERIALIZE_BOOL_N:
	case SERIALIZE_SHORT_INT_N:
	case SERIALIZE_INT_N:
	case SERIALIZE_TIME_N:
	case SERIALIZE_FLOAT_N:
	case SERIALIZE_DOUBLE_N:
	case SERIALIZE_LONG_LONG_N:
	case SERIALIZE_STRING_N:
	case SERIALIZE_ENUM_N:
		if (obj)
			free(obj);
		break;
	case SERIALIZE_ARRAY_N: {
		array *ay = obj;
		if (ay) {
			int i = 0;
			if (ay->isPointe) {
				for (i = 0; i < ay->objectLength; ++i) {
					DerializeFreeN(ay->typeId, *((void**) ay->data + i));
				}
			}
			free(ay->data);
			free(ay);
		}
		break;
	}
	case SERIALIZE_LIST_N: {
		struct serialize_list *list = obj;
		if (list) {
			DerializeFreeN(list->typeId, list->head);
			DerializeFreeN(typeId, list->tail);
			free(list);
		}

		break;
	}
	case SERIALIZE_MAP_N: {
		cache_ptr map = obj;
		if (map) {
			node_ptr np = objc_hash_next(map, NULL);
			int len = map->used;
			hashmapEntry *keys[len];
			int i = 0;
			hashmapEntry *key;
			hashmapEntry *value;
			while (np) {
				key = (hashmapEntry*) np->key;
				keys[i] = key;
				++i;
				value = np->value;
				if (value->typeId > 0) {
					DerializeFreeN(value->typeId, value->data);
				}
				free(value);
				np = objc_hash_next(map, np);
			}
			objc_hash_delete(map);
			for (i = 0; i < len; ++i) {
				key = keys[i];
				DerializeFreeN(key->typeId, key->data);
				free(key);
			}
		}
		break;
	}
	default: {
		if (obj) {
			array *structInfo = objc_hash_value_for_key(structInfoMap, &typeId);
			structFieldInfo *sfi = structInfo->data;
			int fieldLen = structInfo->byteLength / sizeof(structFieldInfo);
			++sfi;
			int i = 0;
			for (i = 1; i < fieldLen; ++i) {
				if (sfi->isPointe) {
					DerializeFreeN(sfi->typeId, *(void**) (obj + sfi->offset));
				}
				++sfi;
			}
			free(obj);
		}
		break;
	}
	}

}
void DerializeFree(char *type, void *obj) {
	if (obj)
		DerializeFreeN(GetTypeId(type), obj);
}

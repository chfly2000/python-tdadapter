/*
 *  $Id: cliviipy.c 2013/10/10 11:00:00 $
 *
 *  This program is free software.
 *  You can distribute/modify this program under the terms of
 *  the Apache License, Version 2.0.
 */

#include "cliviipy.h"

#include <abstract.h>
#include <boolobject.h>
#include <floatobject.h>
#include <intobject.h>
#include <limits.h>
#include <longobject.h>
#include <methodobject.h>
#include <modsupport.h>
#include <object.h>
#include <parcel.h>
#include <pyconfig.h>
#include <pyerrors.h>
#include <pyport.h>
#include <stdio.h>
#include <string.h>
#include <stringobject.h>
#include <tupleobject.h>
#include <structmember.h>

typedef union {
	PclWord SQLLen;
	struct {
		Pclchar Fractional;
		Pclchar Width;
	} DECLen;
} DecLenUnion;

PyObject *unpackInt(CliInfoType *info, CliDataType *data, int *datIdx) {
	PY_LONG_LONG raw;
	PyObject *result;
	char c1;
	short s1;
	long l1;

	switch (info->SQLLen) {
	case 1:
		memcpy(&c1, &data->Data[*datIdx], sizeof(c1));
		raw = c1;
		break;
	case 2:
		memcpy(&s1, &data->Data[*datIdx], sizeof(s1));
		raw = s1;
		break;
	case 4:
		memcpy(&l1, &data->Data[*datIdx], sizeof(l1));
		raw = l1;
		break;
	case 8:
		memcpy(&raw, &data->Data[*datIdx], sizeof(raw));
		break;
	default:
		memcpy(&raw, &data->Data[*datIdx], info->SQLLen);
		break;
	}

	if ((raw >= LONG_MIN) && (raw <= LONG_MAX)) {
		result = PyInt_FromLong(Py_SAFE_DOWNCAST(raw, PY_LONG_LONG, long));
	} else {
		result = PyLong_FromLongLong(raw);
	}
	*datIdx += info->SQLLen;

	return result;
}

PyObject *unpackDate(CliInfoType *info, CliDataType *data, int *datIdx) {
	long raw;
	PyObject *result;

	memcpy(&raw, &data->Data[*datIdx], sizeof(raw));
	raw = raw + 19000000;
	result = PyInt_FromLong(raw);
	*datIdx += info->SQLLen;

	return result;
}

PyObject *unpackDouble(CliInfoType *info, CliDataType *data, int *datIdx) {
	double raw;
	PyObject *result;

	memcpy(&raw, &data->Data[*datIdx], sizeof(raw));
	result = PyFloat_FromDouble(raw);
	*datIdx += info->SQLLen;

	return result;
}

PyObject *unpackDecimal(CliInfoType *info, CliDataType *data, int *datIdx) {
	int i, pos, sign;
	PY_LONG_LONG raw;
	PyObject *result;
	DecLenUnion decLen;
	char buf1[20], buf2[20];
	char c1;
	short s1;
	long l1;

	decLen.SQLLen = info->SQLLen;
	if (decLen.DECLen.Width <= 2) {
		memcpy(&c1, &data->Data[*datIdx], sizeof(c1));
		raw = c1;
		decLen.DECLen.Width = 1;
	} else if (decLen.DECLen.Width <= 4) {
		memcpy(&s1, &data->Data[*datIdx], sizeof(s1));
		raw = s1;
		decLen.DECLen.Width = 2;
	} else if (decLen.DECLen.Width <= 9) {
		memcpy(&l1, &data->Data[*datIdx], sizeof(l1));
		raw = l1;
		decLen.DECLen.Width = 4;
	} else if (decLen.DECLen.Width <= 18) {
		memcpy(&raw, &data->Data[*datIdx], sizeof(raw));
		decLen.DECLen.Width = 8;
	} else {
		decLen.DECLen.Width = 16;
	}

	if (decLen.DECLen.Width == 16) {
		Py_INCREF(Py_None);
		result = Py_None;
	} else {
		sprintf(buf1, "%I64d", raw);
		if (buf1[0] == '-') {
			strcpy(buf2, buf1);
			strcpy(buf1, &buf2[1]);
			sign = 1;
		} else {
			sign = 0;
		}

		if (decLen.DECLen.Fractional > 0) {
			if (strlen(buf1) <= (unsigned char) decLen.DECLen.Fractional) {
				pos = decLen.DECLen.Fractional - strlen(buf1) + 2;
				strcpy(buf2, buf1);
				strcpy(&buf1[pos], buf2);
				for (i = 0; i < pos; i++) {
					buf1[i] = '0';
				}
				buf1[1] = '.';
			} else {
				pos = strlen(buf1) - decLen.DECLen.Fractional;
				strcpy(buf2, &buf1[pos]);
				strcpy(&buf1[pos + 1], buf2);
				buf1[pos] = '.';
			}
		}

		if (sign == 1) {
			strcpy(buf2, buf1);
			strcpy(&buf1[1], buf2);
			buf1[0] = '-';
		}
		result = PyString_FromString(buf1);
	}
	*datIdx += decLen.DECLen.Width;

	return result;
}

PyObject *unpackFixStr(CliInfoType *info, CliDataType *data, int *datIdx,
		char *charset) {
	PyObject *rawstr, *result;

	rawstr = PyString_FromStringAndSize(&data->Data[*datIdx], info->SQLLen);
	if (!rawstr) {
		return NULL;
	}
	result = PyString_AsDecodedObject(rawstr, charset, "");
	*datIdx += info->SQLLen;

	return result;
}

PyObject *unpackVarStr(CliInfoType *info, CliDataType *data, int *datIdx,
		char *charset) {
	PclWord chars;
	PyObject *rawstr, *result;

	memcpy(&chars, &data->Data[*datIdx], sizeof(chars));
	*datIdx += 2;
	rawstr = PyString_FromStringAndSize(&data->Data[*datIdx], chars);
	if (!rawstr) {
		return NULL;
	}
	result = PyString_AsDecodedObject(rawstr, charset, "");
	*datIdx += chars;

	return result;
}

PyObject* unpackField(int fldIdx, CliInfoType *info, CliDataType *data,
		int *datIdx, char *charset) {
	PclByte nullByte;
	int chrIdx, bitIdx;
	PyObject *result = NULL;

	switch (info->SQLType) {
	case 452:
	case 453:
	case 468:
	case 469:
	case 692:
	case 693:
		// Fixed String Type
		result = unpackFixStr(info, data, datIdx, charset);
		break;
	case 496:
	case 497:
		// Fixed Integer Type
		result = unpackInt(info, data, datIdx);
		break;
	case 500:
	case 501:
		// Fixed Short Type
		result = unpackInt(info, data, datIdx);
		break;
	case 600:
	case 601:
		// Fixed Long Type
		result = unpackInt(info, data, datIdx);
		break;
	case 756:
	case 757:
		// Fixed Byte Type
		result = unpackInt(info, data, datIdx);
		break;
	case 480:
	case 481:
		// Fixed Float Types
		result = unpackDouble(info, data, datIdx);
		break;
	case 484:
	case 485:
		// Fixed Decimal Types
		result = unpackDecimal(info, data, datIdx);
		break;
	case 448:
	case 449:
	case 456:
	case 457:
	case 464:
	case 465:
	case 472:
	case 473:
	case 688:
	case 689:
	case 696:
	case 697:
		// Varchar Types
		result = unpackVarStr(info, data, datIdx, charset);
		break;
	case 752:
	case 753:
		// Date Types
		result = unpackDate(info, data, datIdx);
		break;
	}

	if (result != NULL) {
		chrIdx = fldIdx / 8;
		bitIdx = fldIdx % 8;
		memcpy(&nullByte, &data->Data[chrIdx], sizeof(nullByte));
		if (nullByte & (128 >> bitIdx)) {
			Py_DECREF(result);
			Py_INCREF(Py_None);
			result = Py_None;
		}
	}

	return result;
}

PyObject* unpackColumns(CliMetaType meta) {
	int i, idx;
	PclInt16 int16;
	PyObject *value, *result;

	result = PyTuple_New(meta.prep.ColumnCount);
	if (result == NULL) {
		return NULL;
	}

	idx = 0;
	for (i = 0; i < meta.prep.ColumnCount; i++) {
		idx += 2;
		idx += 2;
		memcpy(&int16, &meta.cols[idx], sizeof(int16));
		idx += 2;
		idx += int16;
		memcpy(&int16, &meta.cols[idx], sizeof(int16));
		idx += 2;
		idx += int16;
		memcpy(&int16, &meta.cols[idx], sizeof(int16));
		idx += 2;
		value = PyString_FromStringAndSize(&meta.cols[idx], int16);
		if (value == NULL) {
			Py_DECREF(result);
			return NULL;
		}
		PyTuple_SET_ITEM(result, i, value);
		idx += int16;
	}

	return result;
}

static void CliConn_Dealloc(CliConn* self) {
	bool status = true;

	status = cli_cln(&self->cli);
	if (!status) {
		PyErr_SetString(CliFail, self->cli.errMsg);
	}
	Py_TYPE(self)->tp_free((PyObject*) self);
}

static int CliConn_Init(CliConn* self, PyObject* args, PyObject* kwds) {
	bool status = true;

	status = cli_ini(&self->cli);
	if (!status) {
		PyErr_SetString(CliFail, self->cli.errMsg);
		return -1;
	}

	return 0;
}

static PyObject* CliConn_Enter(CliConn* self, PyObject* args) {
	bool status = true;

	if (self->cli.logged) {
		PyErr_SetString(CliFail, "Already logged on.");
		return NULL;
	}

	status = cli_con(&self->cli);
	if (!status) {
		PyErr_SetString(CliFail, self->cli.errMsg);
		return NULL;
	}

	Py_INCREF(self);
	return (PyObject*) self;
}

static PyObject* CliConn_Exit(CliConn* self, PyObject* args) {
	bool status = true;

	if (!self->cli.logged) {
		PyErr_SetString(CliFail, "Already logged off.");
		return NULL;
	}

	status = cli_dsc(&self->cli);
	if (!status) {
		PyErr_SetString(CliFail, self->cli.errMsg);
		return NULL;
	}

	Py_INCREF(Py_False);
	return Py_False;
}

static PyObject* CliConn_Update(CliConn* self, PyObject* args) {
	long count = 0;
	long status = -1;
	char* query_text;
	PyObject* result;

	if (!PyArg_ParseTuple(args, "s", &query_text)) {
		return NULL;
	}
	if (!self->cli.logged) {
		PyErr_SetString(CliFail, "Already logged off.");
		return NULL;
	}

	count = cli_irq(&self->cli, query_text);
	if (count < 0) {
		PyErr_SetString(CliFail, self->cli.errMsg);
		return NULL;
	}
	status = cli_erq(&self->cli);
	if (status < 0) {
		PyErr_SetString(CliFail, self->cli.errMsg);
		return NULL;
	}

	result = Py_BuildValue("l", count);
	return result;
}

static PyObject* CliConn_Query(CliConn* self, PyObject* args) {
	long count = 0;
	char* query_text;
	PyObject *createArgs, *result;

	if (!PyArg_ParseTuple(args, "s", &query_text)) {
		return NULL;
	}
	if (!self->cli.logged) {
		PyErr_SetString(CliFail, "Already logged off.");
		return NULL;
	}

	count = cli_irq(&self->cli, query_text);
	if (count < 0) {
		PyErr_SetString(CliFail, self->cli.errMsg);
		return NULL;
	}

	createArgs = PyTuple_New(1);
	if (createArgs == NULL) {
		return NULL;
	}
	Py_INCREF(self);
	PyTuple_SET_ITEM(createArgs, 0, (PyObject* ) self);
	result = PyObject_Call((PyObject*) &CliIterType, createArgs, NULL);
	Py_DECREF(createArgs);

	return result;
}

static PyMethodDef CliConn_Methods[] = { { "__enter__",
		(PyCFunction) CliConn_Enter, METH_NOARGS }, { "__exit__",
		(PyCFunction) CliConn_Exit, METH_VARARGS }, { "update",
		(PyCFunction) CliConn_Update, METH_VARARGS }, { "query",
		(PyCFunction) CliConn_Query, METH_VARARGS }, { 0, 0, 0, 0 } };

PyTypeObject CliConnType = { PyObject_HEAD_INIT(NULL) 0, /* ob_size */
"clivii.CliConn", /* tp_name */
sizeof(CliConn), /* tp_basicsize */
0, /* tp_itemsize */
(destructor) CliConn_Dealloc, /* tp_dealloc */
0, /* tp_print */
0, /* tp_getattr */
0, /* tp_setattr */
0, /* tp_compare */
0, /* tp_repr */
0, /* tp_as_number */
0, /* tp_as_sequence */
0, /* tp_as_mapping */
0, /* tp_hash */
0, /* tp_call */
0, /* tp_str */
0, /* tp_getattro */
0, /* tp_setattro */
0, /* tp_as_buffer */
Py_TPFLAGS_DEFAULT, /* tp_flags */
"Teradata Connection Object", /* tp_doc */
0, /* tp_traverse */
0, /* tp_clear */
0, /* tp_richcompare */
0, /* tp_weaklistoffset */
0, /* tp_iter */
0, /* tp_iternext */
CliConn_Methods, /* tp_methods */
0, /* tp_members */
0, /* tp_getset */
0, /* tp_base */
0, /* tp_dict */
0, /* tp_descr_get */
0, /* tp_descr_set */
0, /* tp_dictoffset */
(initproc) CliConn_Init, /* tp_init */
};

static void CliIter_Dealloc(CliIter* self) {
	Py_CLEAR(self->conn);
	Py_CLEAR(self->cols);
	Py_TYPE(self)->tp_free((PyObject*) self);
}

static int CliIter_Init(CliIter* self, PyObject* args, PyObject* kwds) {
	CliConn* conn;

	if (!PyArg_ParseTuple(args, "O!", &CliConnType, &conn)) {
		return -1;
	}
	Py_INCREF(conn);
	self->conn = conn;
	self->cols = NULL;

	return 0;
}

static PyObject* CliIter_Enter(CliIter* self, PyObject* args) {
	long status = -1;
	CliMetaType meta;

	if (!self->conn->cli.logged) {
		PyErr_SetString(CliFail, "Already logged off.");
		return NULL;
	}

	status = cli_fpp(&self->conn->cli, &meta);
	if (status < 0) {
		PyErr_SetString(CliFail, self->conn->cli.errMsg);
		return NULL;
	}
	self->cols = unpackColumns(meta);
	if (self->cols == NULL) {
		PyErr_SetString(CliFail, "Unpack columns failed.");
		return NULL;
	}

	status = cli_fip(&self->conn->cli, &self->head);
	if (status < 0) {
		PyErr_SetString(CliFail, self->conn->cli.errMsg);
		return NULL;
	}

	Py_INCREF(self);
	return (PyObject*) self;
}

static PyObject* CliIter_Next(CliIter* self) {
	int i, idx;
	long status = -1;
	PyObject *value, *result;

	if (!self->conn->cli.logged) {
		PyErr_SetString(CliFail, "Already logged off.");
		return NULL;
	}

	status = cli_frp(&self->conn->cli, &self->data);
	if (status < 0) {
		PyErr_SetString(CliFail, self->conn->cli.errMsg);
		return NULL;
	} else if (status == 0) {
		return NULL;
	}

	result = PyTuple_New(self->head.FieldCount);
	if (result == NULL) {
		return NULL;
	}
	idx = (self->head.FieldCount + 7) / 8;
	for (i = 0; i < self->head.FieldCount; i++) {
		value = unpackField(i, &self->head.InfoVar[i], &self->data, &idx,
				self->conn->cli.charset);
		if (value == NULL) {
			Py_DECREF(result);
			return NULL;
		}
		PyTuple_SET_ITEM(result, i, value);
	}

	return result;
}

static PyObject* CliIter_Exit(CliIter* self, PyObject* args) {
	long status = -1;

	if (!self->conn->cli.logged) {
		PyErr_SetString(CliFail, "Already logged off.");
		return NULL;
	}

	status = cli_erq(&self->conn->cli);
	if (status < 0) {
		PyErr_SetString(CliFail, self->conn->cli.errMsg);
		return NULL;
	}

	Py_INCREF(Py_False);
	return Py_False;
}

static PyMethodDef CliIter_Methods[] = { { "__enter__",
		(PyCFunction) CliIter_Enter, METH_NOARGS }, { "__exit__",
		(PyCFunction) CliIter_Exit, METH_VARARGS }, { 0, 0, 0, 0 } };

static PyMemberDef CliIter_Members[] = { { "cols", T_OBJECT_EX, offsetof(
		CliIter, cols), READONLY, "column names" }, { NULL } };

PyTypeObject CliIterType = { PyObject_HEAD_INIT(NULL) 0, /* ob_size */
"clivii.CliIter", /* tp_name */
sizeof(CliIter), /* tp_basicsize */
0, /* tp_itemsize */
(destructor) CliIter_Dealloc, /* tp_dealloc */
0, /* tp_print */
0, /* tp_getattr */
0, /* tp_setattr */
0, /* tp_compare */
0, /* tp_repr */
0, /* tp_as_number */
0, /* tp_as_sequence */
0, /* tp_as_mapping */
0, /* tp_hash */
0, /* tp_call */
0, /* tp_str */
0, /* tp_getattro */
0, /* tp_setattro */
0, /* tp_as_buffer */
Py_TPFLAGS_DEFAULT, /* tp_flags */
"Teradata Resultset Object", /* tp_doc */
0, /* tp_traverse */
0, /* tp_clear */
0, /* tp_richcompare */
0, /* tp_weaklistoffset */
PyObject_SelfIter, /* tp_iter */
(iternextfunc) CliIter_Next, /* tp_iternext */
CliIter_Methods, /* tp_methods */
CliIter_Members, /* tp_members */
0, /* tp_getset */
0, /* tp_base */
0, /* tp_dict */
0, /* tp_descr_get */
0, /* tp_descr_set */
0, /* tp_dictoffset */
(initproc) CliIter_Init, /* tp_init */
};

static PyObject* CliPy_Conn(PyObject* self, PyObject* args) {
	char *logstr;
	PyObject *createArgs, *result;

	if (!PyArg_ParseTuple(args, "s", &logstr)) {
		return NULL;
	}

	createArgs = PyTuple_New(0);
	if (createArgs == NULL) {
		return NULL;
	}
	result = PyObject_Call((PyObject*) &CliConnType, createArgs, NULL);
	Py_DECREF(createArgs);
	strcpy(((CliConn *) result)->cli.logstr, logstr);
	sprintf(((CliConn *) result)->cli.charset, "%-30s", "UTF8");

	return result;
}

static PyMethodDef clivii_Methods[] = { { "connect", (PyCFunction) CliPy_Conn,
METH_VARARGS }, { 0, 0, 0, 0 } };

#ifndef PyMODINIT_FUNC
#define PyMODINIT_FUNC void
#endif
PyMODINIT_FUNC initclivii(void) {
	PyObject* cliModule;

	CliConnType.tp_new = PyType_GenericNew;
	if (PyType_Ready(&CliConnType) < 0) {
		return;
	}

	CliIterType.tp_new = PyType_GenericNew;
	if (PyType_Ready(&CliIterType) < 0) {
		return;
	}

	cliModule = Py_InitModule3("clivii", clivii_Methods,
			"Python Teradata Access Module.");
	if (cliModule == NULL) {
		return;
	}

	if (PyModule_AddStringConstant(cliModule, "version", COPCLIVersion) < 0) {
		return;
	}

	Py_INCREF(&CliConnType);
	if (PyModule_AddObject(cliModule, "CliConn", (PyObject *) &CliConnType)
			< 0) {
		return;
	}

	Py_INCREF(&CliIterType);
	if (PyModule_AddObject(cliModule, "CliIter", (PyObject *) &CliIterType)
			< 0) {
		return;
	}

	CliFail = PyErr_NewException("clivii.CliFail", NULL, NULL);
	Py_INCREF(CliFail);
	if (PyModule_AddObject(cliModule, "CliFail", CliFail) < 0) {
		return;
	}
}

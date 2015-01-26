/*
 *  $Id: cliviipy.h 2013/10/10 11:00:00 $
 *
 *  This program is free software.
 *  You can distribute/modify this program under the terms of
 *  the Apache License, Version 2.0.
 */

#ifndef CLIVII_PYTHON_H
#define CLIVII_PYTHON_H

#include <Python.h>

#include "clivii.h"

static PyObject* CliFail;

typedef struct {
	PyObject_HEAD
	CliDbcType cli;
} CliConn;

static void CliConn_Dealloc(CliConn* self);
static int CliConn_Init(CliConn* self, PyObject* args, PyObject* kwds);
static PyObject* CliConn_Enter(CliConn* self, PyObject* args);
static PyObject* CliConn_Exit(CliConn* self, PyObject* args);
static PyObject* CliConn_Update(CliConn* self, PyObject* args);
static PyObject* CliConn_Query(CliConn* self, PyObject* args);

extern PyTypeObject CliConnType;

typedef struct {
	PyObject_HEAD
	CliConn* conn;
	PyObject* cols;
	CliHeadType head;
	CliDataType data;
} CliIter;

static void CliIter_Dealloc(CliIter* self);
static int CliIter_Init(CliIter* self, PyObject* args, PyObject* kwds);
static PyObject* CliIter_Enter(CliIter* self, PyObject* args);
static PyObject* CliIter_Next(CliIter* self);
static PyObject* CliIter_Exit(CliIter* self, PyObject* args);

extern PyTypeObject CliIterType;

static PyObject* CliPy_Conn(PyObject* self, PyObject* args);

#endif

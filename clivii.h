/*
 *  $Id: clivii.h 2013/10/10 11:00:00 $
 *
 *  This program is free software.
 *  You can distribute/modify this program under the terms of
 *  the Apache License, Version 2.0.
 */

#ifndef CLIVII_DBCH_H
#define CLIVII_DBCH_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <coptypes.h>
#include <coperr.h>
#include <dbcarea.h>
#include <parcel.h>

#define bool int
#define true 1
#define false 0

typedef struct {
	DBCAREA dbc;
	bool logged;
	char logstr[256];
	char charset[32];
	bool hasErr;
	char errMsg[256 + 6];
} CliDbcType;

typedef struct CliDataInfoType CliHeadType;
typedef struct CliDInfoType CliInfoType;
typedef struct CliIndicDataType CliDataType;

#ifdef WIN32
__declspec(dllimport) char COPCLIVersion[];
#else
extern char COPCLIVersion[];
#endif

bool cli_ini(CliDbcType* cli);
bool cli_cln(CliDbcType* cli);
bool cli_con(CliDbcType* cli);
bool cli_dsc(CliDbcType* cli);

long cli_irq(CliDbcType* cli, char* reqstr);
long cli_erq(CliDbcType* cli);
long cli_fip(CliDbcType* cli, CliHeadType* head);
long cli_frp(CliDbcType* cli, CliDataType* data);

#endif

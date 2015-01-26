/*
 *  $Id: clivii.c 2013/10/10 11:00:00 $
 *
 *  This program is free software.
 *  You can distribute/modify this program under the terms of
 *  the Apache License, Version 2.0.
 */

#include "clivii.h"

char dummy[4];

void seterr(CliDbcType* cli, unsigned short errNo, char* errMsg) {
	int i;
	if (!cli->hasErr) {
		cli->hasErr = true;
		sprintf(cli->errMsg, "%04d", errNo);
		cli->errMsg[4] = ':';
		cli->errMsg[5] = ' ';
		strcpy(&cli->errMsg[6], errMsg);
		for (i = strlen(cli->errMsg) - 1; i >= 0; i--) {
			if (cli->errMsg[i] != '.') {
				cli->errMsg[i] = 0;
			} else {
				break;
			}
		}
	}
}

bool dbchini(CliDbcType* cli) {
	Int32 result = EM_OK;

	cli->dbc.total_len = sizeof(struct DBCAREA);
	DBCHINI(&result, dummy, &cli->dbc);
	if (result != EM_OK) {
		seterr(cli, result, cli->dbc.msg_text);
		return false;
	}
	cli->dbc.i_sess_id = cli->dbc.o_sess_id;
	cli->dbc.i_req_id = cli->dbc.o_req_id;

	return true;
}

bool dbchcln(CliDbcType* cli) {
	Int32 result = EM_OK;

	DBCHCLN(&result, dummy);
	if (result != EM_OK) {
		seterr(cli, result, cli->dbc.msg_text);
		return false;
	}

	return true;
}

bool dbfcon(CliDbcType* cli) {
	Int32 result = EM_OK;

	cli->dbc.logon_ptr = cli->logstr;
	cli->dbc.logon_len = (UInt32) strlen(cli->logstr);
	cli->dbc.func = DBFCON;
	DBCHCL(&result, dummy, &cli->dbc);
	if (result != EM_OK) {
		seterr(cli, result, cli->dbc.msg_text);
		return false;
	}
	cli->logged = true;
	cli->dbc.i_sess_id = cli->dbc.o_sess_id;
	cli->dbc.i_req_id = cli->dbc.o_req_id;

	return true;
}

bool dbfdsc(CliDbcType* cli) {
	Int32 result = EM_OK;

	cli->dbc.func = DBFDSC;
	DBCHCL(&result, dummy, &cli->dbc);
	if (result != EM_OK) {
		seterr(cli, result, cli->dbc.msg_text);
		return false;
	}
	cli->dbc.i_sess_id = 0;
	cli->dbc.i_req_id = 0;
	cli->logged = false;

	return true;
}

bool dbfirq(CliDbcType* cli, char* reqstr) {
	Int32 result = EM_OK;

	cli->dbc.req_ptr = reqstr;
	cli->dbc.req_len = (UInt32) strlen(reqstr);
	cli->dbc.func = DBFIRQ;
	DBCHCL(&result, dummy, &cli->dbc);
	if (result != EM_OK) {
		seterr(cli, result, cli->dbc.msg_text);
		return false;
	}
	cli->dbc.i_req_id = cli->dbc.o_req_id;

	return true;
}

bool dbferq(CliDbcType* cli) {
	Int32 result = EM_OK;

	cli->dbc.func = DBFERQ;
	DBCHCL(&result, dummy, &cli->dbc);
	if (result != EM_OK) {
		seterr(cli, result, cli->dbc.msg_text);
		return false;
	}
	cli->dbc.i_req_id = 0;

	return true;
}

bool cli_ini(CliDbcType* cli) {
	cli->hasErr = false;
	if (!dbchini(cli)) {
		dbchcln(cli);
		return false;
	}

	cli->dbc.change_opts = 'Y';
	cli->dbc.resp_mode = 'I';
	cli->dbc.use_presence_bits = 'N';
	cli->dbc.keep_resp = 'N';
	cli->dbc.wait_across_crash = 'Y';
	cli->dbc.tell_about_crash = 'Y';
	cli->dbc.loc_mode = 'Y';
	cli->dbc.var_len_req = 'N';
	cli->dbc.var_len_fetch = 'N';
	cli->dbc.save_resp_buf = 'N';
	cli->dbc.two_resp_bufs = 'N';
	cli->dbc.ret_time = 'N';
	cli->dbc.parcel_mode = 'Y';
	cli->dbc.wait_for_resp = 'Y';
	cli->dbc.req_proc_opt = 'E';
	cli->dbc.charset_type = 'N';
	cli->dbc.inter_ptr = cli->charset;

	return true;
}

bool cli_cln(CliDbcType* cli) {
	cli->hasErr = false;
	if (cli->logged) {
		dbfdsc(cli);
	}
	if (!dbchcln(cli)) {
		return false;
	}

	return true;
}

bool cli_con(CliDbcType* cli) {
	Int32 result = EM_OK;
	struct CliFailureType* failPcl = NULL;

	cli->hasErr = false;
	if (!dbfcon(cli)) {
		return false;
	}

	cli->dbc.func = DBFFET;
	DBCHCL(&result, dummy, &cli->dbc);
	if (result != EM_OK) {
		seterr(cli, result, cli->dbc.msg_text);
		dbfdsc(cli);
		return false;
	}
	switch (cli->dbc.fet_parcel_flavor) {
	case PclFAILURE :
	case PclERROR :
		failPcl = (struct CliFailureType*) cli->dbc.fet_data_ptr;
		seterr(cli, failPcl->Code, failPcl->Msg);
		dbfdsc(cli);
		return false;
	}

	if (!dbferq(cli)) {
		dbfdsc(cli);
		return false;
	}

	return true;
}

bool cli_dsc(CliDbcType* cli) {
	cli->hasErr = false;
	if (!dbfdsc(cli)) {
		return false;
	}

	return true;
}

long cli_irq(CliDbcType* cli, char* reqstr) {
	int count = -1, i;
	Int32 result = EM_OK;
	struct CliSuccessType* succPcl = NULL;
	struct CliFailureType* failPcl = NULL;

	cli->hasErr = false;
	for (i = 0; i < strlen(reqstr); i++) {
		if (reqstr[i] == '\n') {
			if ((i > 0) && (reqstr[i - 1] == '\r')) {
				reqstr[i] = ' ';
			} else {
				reqstr[i] = '\r';
			}
		}
	}
	printf(reqstr);
	if (!dbfirq(cli, reqstr)) {
		return -1;
	}

	cli->dbc.func = DBFFET;
	DBCHCL(&result, dummy, &cli->dbc);
	if (result != EM_OK) {
		seterr(cli, result, cli->dbc.msg_text);
		dbferq(cli);
		count = -1;
	} else {
		switch (cli->dbc.fet_parcel_flavor) {
		case PclSUCCESS :
			succPcl = (struct CliSuccessType*) cli->dbc.fet_data_ptr;
			memcpy(&count, succPcl->ActivityCount, sizeof(int));
			break;
		case PclFAILURE :
		case PclERROR :
			failPcl = (struct CliFailureType*) cli->dbc.fet_data_ptr;
			seterr(cli, failPcl->Code, failPcl->Msg);
			dbferq(cli);
			count = -1;
		}
	}

	return count;
}

long cli_erq(CliDbcType* cli) {
	cli->hasErr = false;
	if (!dbferq(cli)) {
		return -1;
	}

	return 0;
}

long cli_fip(CliDbcType* cli, CliHeadType* head) {
	Int32 result = EM_OK;
	struct CliFailureType* failPcl = NULL;

	cli->hasErr = false;
	while (true) {
		cli->dbc.func = DBFFET;
		DBCHCL(&result, dummy, &cli->dbc);
		if (result == REQEXHAUST) {
			return 0;
		} else if (result != EM_OK) {
			seterr(cli, result, cli->dbc.msg_text);
			return -1;
		} else {
			switch (cli->dbc.fet_parcel_flavor) {
			case PclDATAINFO :
				memcpy(head, cli->dbc.fet_data_ptr, cli->dbc.fet_ret_data_len);
				return cli->dbc.fet_ret_data_len;
			case PclFAILURE :
			case PclERROR :
				failPcl = (struct CliFailureType*) cli->dbc.fet_data_ptr;
				seterr(cli, failPcl->Code, failPcl->Msg);
				return -1;
			case PclENDSTATEMENT :
			case PclENDREQUEST :
				return 0;
			}
		}
	}

	return -1;
}

long cli_frp(CliDbcType* cli, CliDataType* data) {
	Int32 result = EM_OK;
	struct CliFailureType* failPcl = NULL;

	cli->hasErr = false;
	while (true) {
		cli->dbc.func = DBFFET;
		DBCHCL(&result, dummy, &cli->dbc);
		if (result == REQEXHAUST) {
			return 0;
		} else if (result != EM_OK) {
			seterr(cli, result, cli->dbc.msg_text);
			return -1;
		} else {
			switch (cli->dbc.fet_parcel_flavor) {
			case PclRECORD :
				memcpy(data, cli->dbc.fet_data_ptr, cli->dbc.fet_ret_data_len);
				return cli->dbc.fet_ret_data_len;
			case PclFAILURE :
			case PclERROR :
				failPcl = (struct CliFailureType*) cli->dbc.fet_data_ptr;
				seterr(cli, failPcl->Code, failPcl->Msg);
				return -1;
			case PclENDSTATEMENT :
			case PclENDREQUEST :
				return 0;
			}
		}
	}

	return -1;
}

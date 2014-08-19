
#ifndef UTIL_H_INCLUDED
#define UTIL_H_INCLUDED

#include <stdio.h>	/* For snprintf(3) */
#include <stdlib.h>	/* For *alloc(3) */
#include <string.h>	/* For memcpy(3) */
#include <sys/types.h>	/* For size_t */
#include <stdarg.h>	/* For va_start */
#include <stddef.h>	/* for offsetof and ptrdiff_t */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif 

#ifdef _WIN32
#ifndef WIN32
#define WIN32
#endif
#endif

#ifdef	WIN32

#include <malloc.h>
#include <stdint.h>
#define	 vsnprintf	_vsnprintf
#define	 snprintf	_snprintf
#define sleep(x) Sleep(1000*x)

#ifdef _MSC_VER			/* MSVS.Net */
#ifndef __cplusplus
#define inline __inline
#endif
#define	ssize_t		SSIZE_T
//typedef	char		int8_t;
//typedef	short		int16_t;
//typedef	int		int32_t;
//typedef	unsigned char	uint8_t;
//typedef	unsigned short	uint16_t;
//typedef	unsigned int	uint32_t;
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <float.h>
#define isnan _isnan
#define finite _finite
#define copysign _copysign
#define	ilogb	_logb
#endif	/* _MSC_VER */

#else	/* !WIN32 */

#if defined(__vxworks)
#include <types/vxTypes.h>
#else	/* !defined(__vxworks) */

#include <inttypes.h>	/* C99 specifies this file */
/*
 * 1. Earlier FreeBSD version didn't have <stdint.h>,
 * but <inttypes.h> was present.
 * 2. Sun Solaris requires <alloca.h> for alloca(3),
 * but does not have <stdint.h>.
 */
#if	(!defined(__FreeBSD__) || !defined(_SYS_INTTYPES_H_))
#if	defined(sun)
#include <alloca.h>	/* For alloca(3) */
#include <ieeefp.h>	/* for finite(3) */
#elif	defined(__hpux)
#ifdef	__GNUC__
#include <alloca.h>	/* For alloca(3) */
#else	/* !__GNUC__ */
#define inline
#endif	/* __GNUC__ */
#else
#include <stdint.h>	/* SUSv2+ and C99 specify this file, for uintXX_t */
#endif	/* defined(sun) */
#endif

#endif	/* defined(__vxworks) */

#endif	/* WIN32 */

#if	__GNUC__ >= 3
#ifndef	GCC_PRINTFLIKE
#define	GCC_PRINTFLIKE(fmt,var)	__attribute__((format(printf,fmt,var)))
#endif
#else
#ifndef	GCC_PRINTFLIKE
#define	GCC_PRINTFLIKE(fmt,var)	/* nothing */
#endif
#endif

#ifndef	offsetof	/* If not defined by <stddef.h> */
#define	offsetof(s, m)	((ptrdiff_t)&(((s *)0)->m) - (ptrdiff_t)((s *)0))
#endif	/* offsetof */

#ifndef	MIN		/* Suitable for comparing primitive types (integers) */
#if defined(__GNUC__)
#define	MIN(a,b)	({ __typeof a _a = a; __typeof b _b = b;	\
	((_a)<(_b)?(_a):(_b)); })
#else	/* !__GNUC__ */
#define	MIN(a,b)	((a)<(b)?(a):(b))	/* Unsafe variant */
#endif /* __GNUC__ */
#endif	/* MIN */


#include <time.h>
#include <string.h>
#include "mms_types.h"
#include "mms_value_internal.h"
#include "mms_client_connection.h"

void write_dataset(MmsConnection con, char * id_iccp, char * ds_name, char * ts_name, int buffer_time, int integrity_time, int all_changes_reported);

MmsValue * get_next_transferset(MmsConnection con, char * id_iccp, FILE * error_file);

int check_connection(MmsConnection con, char * id_iccp, FILE *    error_file);

int connect_to_server(MmsConnection con, char * server);

int command_variable(MmsConnection con, FILE *error_file, char * varibale, int value);

static inline void print_value (char state, bool ana, time_t time_stamp, unsigned short time_stamp_extended, char * state_on, char * state_off) {
	struct tm * time_result;
	MmsValue * value = MmsValue_newBitString(8);
	memcpy(value->value.bitString.buf, &state, 1);

	//DEBUG
	/*	printf("State_hi %d State_lo %d, Validity_hi %d, Validity_lo %d, CurrentSource_hi %d, CurrentSource_lo %d, NormalValue %d, TimeStampQuality %d \n", 
		MmsValue_getBitStringBit(value,0), MmsValue_getBitStringBit(value,1), MmsValue_getBitStringBit(value,2),
		MmsValue_getBitStringBit(value,3), MmsValue_getBitStringBit(value,4), MmsValue_getBitStringBit(value,5),
		MmsValue_getBitStringBit(value,6), MmsValue_getBitStringBit(value,7) );
	 */
	if (!ana) {
		//ESTADO
		if (MmsValue_getBitStringBit(value,0) && !MmsValue_getBitStringBit(value,1)) {
			printf("1-%-16s |", state_off);
		}else if (!MmsValue_getBitStringBit(value,0) && MmsValue_getBitStringBit(value,1)) {
			printf("0-%-16s |", state_on);
		} else {
			printf ("       Invalid    |");
		}
	}

	//Validade
	if (!MmsValue_getBitStringBit(value,2) && !MmsValue_getBitStringBit(value,3)) {
		printf("Valido   |");
	}else if (!MmsValue_getBitStringBit( value,2) && MmsValue_getBitStringBit(value,3)) {
		printf("Segurado |");
	}else if (MmsValue_getBitStringBit(value,2) && !MmsValue_getBitStringBit(value,3)) {
		printf("Suspeito |");
	} else {
		printf("Inválido |");
	}

	// Origem
	if (!MmsValue_getBitStringBit(value,4) && !MmsValue_getBitStringBit(value,5)) {
		printf("Telemedido |");
	}else if (!MmsValue_getBitStringBit(value,4) && MmsValue_getBitStringBit(value,5)) {
		printf("Calculado  |");
	}else if (MmsValue_getBitStringBit(value,4) && !MmsValue_getBitStringBit(value,5)) {
		printf("Manual     |");
	} else {
		printf ("Estimado  |");
	}

	// Valor Normal
	if (!MmsValue_getBitStringBit(value,6)){
		printf ("Normal  |");
	} else {
		printf ("Anormal |");
	}

	// Estampa de tempo
	if (!MmsValue_getBitStringBit(value,7)){
		printf ("T Valida   |");
	} else {
		printf ("T Invalida |");
	}

	time_result = localtime(&time_stamp);
	if(!ana)
		printf("%d:%s ", time_stamp_extended, asctime(time_result));
	else
		printf("%s ", asctime(time_result));

}

#endif

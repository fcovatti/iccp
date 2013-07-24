/*
 * Generated by asn1c-0.9.21 (http://lionet.info/asn1c)
 * From ASN.1 module "ISO8823PRESENTATION"
 * 	found in "../isoPresentationLayer.asn"
 * 	`asn1c -fskeletons-copy`
 */

#ifndef	_CPtype_H_
#define	_CPtype_H_


#include <asn_application.h>

/* Including external dependencies */
#include "Modeselector.h"
#include "Protocolversion.h"
#include "Callingpresentationselector.h"
#include "Calledpresentationselector.h"
#include "Presentationrequirements.h"
#include "Usersessionrequirements.h"
#include <constr_SEQUENCE.h>
#include <constr_SET.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Dependencies */

/*
 * Method of determining the components presence
 */
typedef enum CPtype_PR {
	CPtype_PR_modeselector,	/* Member modeselector is present */
	CPtype_PR_normalmodeparameters,	/* Member normalmodeparameters is present */
} CPtype_PR;

/* Forward declarations */
struct Contextlist;
struct Defaultcontextname;
struct Userdata;

/* CPtype */
typedef struct CPtype {
	Modeselector_t	 modeselector;
	struct normalmodeparameters {
		Protocolversion_t	*protocolversion	/* DEFAULT {version1} */;
		Callingpresentationselector_t	*callingpresentationselector	/* OPTIONAL */;
		Calledpresentationselector_t	*calledpresentationselector	/* OPTIONAL */;
		struct Contextlist	*presentationcontextdefinitionlist	/* OPTIONAL */;
		struct Defaultcontextname	*defaultcontextname	/* OPTIONAL */;
		Presentationrequirements_t	*presentationrequirements	/* OPTIONAL */;
		Usersessionrequirements_t	*usersessionrequirements	/* OPTIONAL */;
		struct Userdata	*userdata	/* OPTIONAL */;
		
		/* Context for parsing across buffer boundaries */
		asn_struct_ctx_t _asn_ctx;
	} *normalmodeparameters;
	
	/* Presence bitmask: ASN_SET_ISPRESENT(pCPtype, CPtype_PR_x) */
	unsigned int _presence_map
		[((2+(8*sizeof(unsigned int))-1)/(8*sizeof(unsigned int)))];
	
	/* Context for parsing across buffer boundaries */
	asn_struct_ctx_t _asn_ctx;
} CPtype_t;

/* Implementation */
extern asn_TYPE_descriptor_t asn_DEF_CPtype;

#ifdef __cplusplus
}
#endif

/* Referred external types */
#include "Presentationcontextdefinitionlist.h"
#include "Defaultcontextname.h"
#include "Userdata.h"

#endif	/* _CPtype_H_ */

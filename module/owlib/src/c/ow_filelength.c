/*
$Id$
    OWFS -- One-Wire filesystem
    OWHTTPD -- One-Wire Web Server
    Written 2003 Paul H Alfille
    email: palfille@earthlink.net
    Released under the GPL
    See the header file: ow.h for full attribution
    1wire/iButton system from Dallas Semiconductor
*/

#include <config.h>
#include "owfs_config.h"
#include "ow.h"

size_t OWQ_FileLength( struct one_wire_query * owq )
{
    if (OWQ_pn(owq).type == pn_structure)
        return 30;              /* longest seem to be /1wire/structure/0F/memory.ALL (28 bytes) so far... */

    /* directory ? */
    if (IsDir(&OWQ_pn(owq)))
        return 8;

    switch ( OWQ_pn(owq).ft->format ) {
        case ft_yesno:
            return PROPERTY_LENGTH_YESNO ;
        case ft_integer:
            return PROPERTY_LENGTH_INTEGER ;
        case ft_unsigned:
            return PROPERTY_LENGTH_UNSIGNED ;
        case ft_float:
        case ft_temperature:
        case ft_tempgap:
            return PROPERTY_LENGTH_FLOAT ;
        case ft_date:
            return PROPERTY_LENGTH_DATE ;
        case ft_bitfield:
            return (OWQ_pn(owq).extension==EXTENSION_BYTE) ? PROPERTY_LENGTH_UNSIGNED : PROPERTY_LENGTH_YESNO ;
        case ft_vascii:
        case ft_ascii:
        case ft_binary:
        default:
            return OWQ_pn(owq).ft->suglen ; ;
    }
}

    /* Length of file based on filetype and extension */
size_t OWQ_FullFileLength( struct one_wire_query * owq )
{
    if (OWQ_pn(owq).type == pn_structure) {
        return OWQ_FileLength(owq) ;
    } else if (OWQ_pn(owq).extension != EXTENSION_ALL ) {
        return OWQ_FileLength(owq) ;
    } else {
        size_t elements = OWQ_pn(owq).ft->ag->elements ;
        if ( OWQ_pn(owq).ft->format==ft_binary ) {
            return OWQ_FileLength(owq) * elements ;
        } else { // add room for commas
            return (OWQ_FileLength(owq) + 1) * elements  - 1 ;
        }
    }
}

/* Length of file based on filetype alone */
size_t FileLength(const struct parsedname * pn)
{
	/* structure ? */
	if (pn->type == pn_structure)
		return 30;				/* longest seem to be /1wire/structure/0F/memory.ALL (28 bytes) so far... */

	/* directory ? */
    if (IsDir(pn))
		return 8;

	/* local or simple remote, test for special case */
	switch (pn->ft->format) {
		/* bitfield ? */
	case ft_bitfield:
		if (pn->extension == EXTENSION_BYTE)
			return 12;
		break;
	case ft_vascii:			// variable length ascii
		{
            struct one_wire_query owq ;
            OWQ_size(&owq) = pn->ft->suglen ;
            OWQ_offset(&owq) = 0 ;
            OWQ_buffer(&owq) = malloc(OWQ_size(&owq));
            memcpy( &OWQ_pn(&owq), pn, sizeof(struct parsedname) ) ;
            if (OWQ_buffer(&owq)) {
				ssize_t ret = FS_read_postpostparse(&owq);
                free(OWQ_buffer(&owq));
				if (ret > 0)
					return ret;
			}
            return OWQ_size(&owq) ;
		}
		/* fall through is ok */
	default:
		break;
	}
	return pn->ft->suglen;
}

/* Length of file based on filetype and extension */
size_t FullFileLength(const struct parsedname * pn)
{
	if (pn->type == pn_structure)
		return 30;
	/* longest seem to be /1wire/structure/0F/memory.ALL (28 bytes) so far... */
	//printf("FullFileLength: pid=%ld %s\n", pthread_self(), pn->path);
	if (pn->ft && pn->ft->ag) {	/* aggregate files */
		switch (pn->extension) {
		case EXTENSION_ALL:				/* ALL */
			if ((pn->ft->format == ft_binary)
				|| (pn->ft->format == ft_ascii)) {
				/* not comma-separated values are ft_binary and ft_ascii
				 * ft_binary is used for all memory devices
				 * ft_ascii is used in ow_lcd.c:line16.ALL */
				//printf("FullFileLength: pid=%ld size1=%d\n", pthread_self(), (pn->ft->ag->elements) * (pn->ft->suglen) );
				return (pn->ft->ag->elements) * (pn->ft->suglen);
			} else {
				/* comma separated, but does not end with a comma
				 * used in ow_lcd.c:gpio.ALL which is ft_yesno for example */
				//printf("FullFileLength: pid=%ld size2=%d\n", pthread_self(), ((pn->ft->ag->elements) * (pn->ft->suglen + 1)) - 1 );
				return ((pn->ft->ag->elements) * (pn->ft->suglen + 1)) - 1;
			}
		case EXTENSION_BYTE:				/* BYTE */
			return 12;
		}
	}
	/* default simple file */
	return FileLength(pn);
}

/* Length of file based on filetype alone */
size_t SimpleFileLength(const struct parsedname * pn)
{
	/* structure ? */
	if (pn->type == pn_structure)
		return 30;				/* longest seem to be /1wire/structure/0F/memory.ALL (28 bytes) so far... */

	/* directory ? */
	if (pn->dev == NULL || pn->ft == NULL)
		return 8;
	if (pn->ft->format == ft_directory || pn->ft->format == ft_subdir)
		return 8;				/* arbitrary, but non-zero for "find" and "tree" commands */

	/* local or simple remote, test for special case */
	switch (pn->ft->format) {
		/* bitfield ? */
	case ft_bitfield:
		if (pn->extension == EXTENSION_BYTE)
			return 12;
		break;
	default:
		break;
	}
	return pn->ft->suglen;
}

/* Length of file based on filetype and extension */
size_t SimpleFullFileLength(const struct parsedname * pn)
{
	if (pn->type == pn_structure)
		return 30;
	/* longest seem to be /1wire/structure/0F/memory.ALL (28 bytes) so far... */
	//printf("FullFileLength: pid=%ld %s\n", pthread_self(), pn->path);
	if (pn->ft && pn->ft->ag) {	/* aggregate files */
		switch (pn->extension) {
		case EXTENSION_ALL:				/* ALL */
			if ((pn->ft->format == ft_binary)
				|| (pn->ft->format == ft_ascii)) {
				/* not comma-separated values are ft_binary and ft_ascii
				 * ft_binary is used for all memory devices
				 * ft_ascii is used in ow_lcd.c:line16.ALL */
				//printf("FullFileLength: pid=%ld size1=%d\n", pthread_self(), (pn->ft->ag->elements) * (pn->ft->suglen) );
				return (pn->ft->ag->elements) * (pn->ft->suglen);
			} else {
				/* comma separated, but does not end with a comma
				 * used in ow_lcd.c:gpio.ALL which is ft_yesno for example */
				//printf("FullFileLength: pid=%ld size2=%d\n", pthread_self(), ((pn->ft->ag->elements) * (pn->ft->suglen + 1)) - 1 );
				return ((pn->ft->ag->elements) * (pn->ft->suglen + 1)) - 1;
			}
		case EXTENSION_BYTE:				/* BYTE */
			return 12;
		}
	}
	/* default simple file */
	return SimpleFileLength(pn);
}

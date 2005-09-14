/* $Id$ */
/**************************************************************************
 *   chars.c                                                              *
 *                                                                        *
 *   Copyright (C) 2005 Chris Allegretta                                  *
 *   This program is free software; you can redistribute it and/or modify *
 *   it under the terms of the GNU General Public License as published by *
 *   the Free Software Foundation; either version 2, or (at your option)  *
 *   any later version.                                                   *
 *                                                                        *
 *   This program is distributed in the hope that it will be useful, but  *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of           *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU    *
 *   General Public License for more details.                             *
 *                                                                        *
 *   You should have received a copy of the GNU General Public License    *
 *   along with this program; if not, write to the Free Software          *
 *   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA            *
 *   02110-1301, USA.                                                     *
 *                                                                        *
 **************************************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <ctype.h>
#include "proto.h"

#ifdef ENABLE_UTF8
#ifdef HAVE_WCHAR_H
#include <wchar.h>
#endif
#ifdef HAVE_WCTYPE_H
#include <wctype.h>
#endif

static const wchar_t bad_wchar = 0xFFFD;
	/* If we get an invalid multibyte sequence, we treat it as
	 * Unicode FFFD (Replacement Character), unless we're
	 * determining if it's a control character or searching for a
	 * match to it. */
static const char *bad_mbchar = "\xEF\xBF\xBD";
static const int bad_mbchar_len = 3;
#endif

#ifndef HAVE_ISBLANK
/* This function is equivalent to isblank(). */
bool nisblank(int c)
{
    return isspace(c) && (c == '\t' || !is_cntrl_char(c));
}
#endif

#if !defined(HAVE_ISWBLANK) && defined(ENABLE_UTF8)
/* This function is equivalent to iswblank(). */
bool niswblank(wchar_t wc)
{
    return iswspace(wc) && (wc == '\t' || !is_cntrl_wchar(wc));
}
#endif

/* Return TRUE if the value of c is in byte range, and FALSE
 * otherwise. */
bool is_byte(int c)
{
    return ((unsigned int)c == (unsigned char)c);
}

/* This function is equivalent to isalnum() for multibyte characters. */
bool is_alnum_mbchar(const char *c)
{
    assert(c != NULL);

#ifdef ENABLE_UTF8
    if (ISSET(USE_UTF8)) {
	wchar_t wc;

	if (mbtowc(&wc, c, MB_CUR_MAX) < 0) {
	    mbtowc(NULL, NULL, 0);
	    wc = bad_wchar;
	}

	return iswalnum(wc);
    } else
#endif
	return isalnum((unsigned char)*c);
}

/* This function is equivalent to isblank() for multibyte characters. */
bool is_blank_mbchar(const char *c)
{
    assert(c != NULL);

#ifdef ENABLE_UTF8
    if (ISSET(USE_UTF8)) {
	wchar_t wc;

	if (mbtowc(&wc, c, MB_CUR_MAX) < 0) {
	    mbtowc(NULL, NULL, 0);
	    wc = bad_wchar;
	}

	return iswblank(wc);
    } else
#endif
	return isblank((unsigned char)*c);
}

/* This function is equivalent to iscntrl(), except in that it also
 * handles high-bit control characters. */
bool is_cntrl_char(int c)
{
    return (-128 <= c && c < -96) || (0 <= c && c < 32) ||
	(127 <= c && c < 160);
}

#ifdef ENABLE_UTF8
/* This function is equivalent to iscntrl() for wide characters, except
 * in that it also handles wide control characters with their high bits
 * set. */
bool is_cntrl_wchar(wchar_t wc)
{
    return (0 <= wc && wc < 32) || (127 <= wc && wc < 160);
}
#endif

/* This function is equivalent to iscntrl() for multibyte characters,
 * except in that it also handles multibyte control characters with
 * their high bits set. */
bool is_cntrl_mbchar(const char *c)
{
    assert(c != NULL);

#ifdef ENABLE_UTF8
    if (ISSET(USE_UTF8)) {
	wchar_t wc;

	if (mbtowc(&wc, c, MB_CUR_MAX) < 0) {
	    mbtowc(NULL, NULL, 0);
	    wc = bad_wchar;
	}

	return is_cntrl_wchar(wc);
    } else
#endif
	return is_cntrl_char((unsigned char)*c);
}

/* This function is equivalent to ispunct() for multibyte characters. */
bool is_punct_mbchar(const char *c)
{
    assert(c != NULL);

#ifdef ENABLE_UTF8
    if (ISSET(USE_UTF8)) {
	wchar_t wc;
	int c_mb_len = mbtowc(&wc, c, MB_CUR_MAX);

	if (c_mb_len < 0) {
	    mbtowc(NULL, NULL, 0);
	    wc = bad_wchar;
	}

	return iswpunct(wc);
    } else
#endif
	return ispunct((unsigned char)*c);
}

/* Return TRUE for a multibyte character found in a word (currently only
 * an alphanumeric or punctuation character, and only the latter if
 * allow_punct is TRUE) and FALSE otherwise. */
bool is_word_mbchar(const char *c, bool allow_punct)
{
    assert(c != NULL);

    return is_alnum_mbchar(c) || (allow_punct ? is_punct_mbchar(c) :
	FALSE);
}

/* c is a control character.  It displays as ^@, ^?, or ^[ch], where ch
 * is (c + 64).  We return that character. */
char control_rep(char c)
{
    assert(is_cntrl_char(c));

    /* Treat newlines embedded in a line as encoded nulls. */
    if (c == '\n')
	return '@';
    else if (c == NANO_CONTROL_8)
	return '?';
    else
	return c + 64;
}

#ifdef ENABLE_UTF8
/* c is a wide control character.  It displays as ^@, ^?, or ^[ch],
 * where ch is (c + 64).  We return that wide character. */
wchar_t control_wrep(wchar_t wc)
{
    assert(is_cntrl_wchar(wc));

    /* Treat newlines embedded in a line as encoded nulls. */
    if (wc == '\n')
	return '@';
    else if (wc == NANO_CONTROL_8)
	return '?';
    else
	return wc + 64;
}
#endif

/* c is a multibyte control character.  It displays as ^@, ^?, or ^[ch],
 * where ch is (c + 64).  We return that multibyte character.  If crep
 * is an invalid multibyte sequence, it will be replaced with Unicode
 * 0xFFFD (Replacement Character). */
char *control_mbrep(const char *c, char *crep, int *crep_len)
{
    assert(c != NULL && crep != NULL && crep_len != NULL);

#ifdef ENABLE_UTF8
    if (ISSET(USE_UTF8)) {
	wchar_t wc;

	if (mbtowc(&wc, c, MB_CUR_MAX) < 0) {
	    mbtowc(NULL, NULL, 0);
	    *crep_len = bad_mbchar_len;
	    strncpy(crep, bad_mbchar, *crep_len);
	} else {
	    *crep_len = wctomb(crep, control_wrep(wc));

	    if (*crep_len < 0) {
		wctomb(NULL, 0);
		*crep_len = 0;
	    }
	}
    } else {
#endif
	*crep_len = 1;
	*crep = control_rep(*c);
#ifdef ENABLE_UTF8
    }
#endif

    return crep;
}

/* c is a multibyte non-control character.  We return that multibyte
 * character.  If crep is an invalid multibyte sequence, it will be
 * replaced with Unicode 0xFFFD (Replacement Character). */
char *mbrep(const char *c, char *crep, int *crep_len)
{
    assert(c != NULL && crep != NULL && crep_len != NULL);

#ifdef ENABLE_UTF8
    if (ISSET(USE_UTF8)) {
	wchar_t wc;

	/* Reject invalid Unicode characters. */
	if (mbtowc(&wc, c, MB_CUR_MAX) < 0 || !is_valid_unicode(wc)) {
	    mbtowc(NULL, NULL, 0);
	    *crep_len = bad_mbchar_len;
	    strncpy(crep, bad_mbchar, *crep_len);
	} else {
	    *crep_len = wctomb(crep, wc);

	    if (*crep_len < 0) {
		wctomb(NULL, 0);
		*crep_len = 0;
	    }
	}
    } else {
#endif
	*crep_len = 1;
	*crep = *c;
#ifdef ENABLE_UTF8
    }
#endif

    return crep;
}

/* This function is equivalent to wcwidth() for multibyte characters. */
int mbwidth(const char *c)
{
    assert(c != NULL);

#ifdef ENABLE_UTF8
    if (ISSET(USE_UTF8)) {
	wchar_t wc;
	int width;

	if (mbtowc(&wc, c, MB_CUR_MAX) < 0) {
	    mbtowc(NULL, NULL, 0);
	    wc = bad_wchar;
	}

	width = wcwidth(wc);

	if (width == -1)
	    width++;

	return width;
    } else
#endif
	return 1;
}

/* Return the maximum width in bytes of a multibyte character. */
int mb_cur_max(void)
{
    return
#ifdef ENABLE_UTF8
	ISSET(USE_UTF8) ? MB_CUR_MAX :
#endif
	1;
}

/* Convert the Unicode value in chr to a multibyte character with the
 * same wide character value as chr, if possible.  If the conversion
 * succeeds, return the (dynamically allocated) multibyte character and
 * its length.  Otherwise, return an undefined (dynamically allocated)
 * multibyte character and a length of zero. */
char *make_mbchar(long chr, int *chr_mb_len)
{
    char *chr_mb;

    assert(chr_mb_len != NULL);

#ifdef ENABLE_UTF8
    if (ISSET(USE_UTF8)) {
	chr_mb = charalloc(MB_CUR_MAX);
	*chr_mb_len = wctomb(chr_mb, (wchar_t)chr);

	/* Reject invalid Unicode characters. */
	if (*chr_mb_len < 0 || !is_valid_unicode((wchar_t)chr)) {
	    wctomb(NULL, 0);
	    *chr_mb_len = 0;
	}
    } else {
#endif
	*chr_mb_len = 1;
	chr_mb = mallocstrncpy(NULL, (char *)&chr, 1);
#ifdef ENABLE_UTF8
    }
#endif

    return chr_mb;
}

/* Parse a multibyte character from buf.  Return the number of bytes
 * used.  If chr isn't NULL, store the multibyte character in it.  If
 * col isn't NULL, store the new display width in it.  If *buf is '\t',
 * we expect col to have the current display width. */
int parse_mbchar(const char *buf, char *chr, size_t *col)
{
    int buf_mb_len;

    assert(buf != NULL);

#ifdef ENABLE_UTF8
    if (ISSET(USE_UTF8)) {
	/* Get the number of bytes in the multibyte character. */
	buf_mb_len = mblen(buf, MB_CUR_MAX);

	/* If buf contains an invalid multibyte character, set bad_chr
	 * to TRUE and interpret buf's first byte. */
	if (buf_mb_len < 0) {
	    mblen(NULL, 0);
	    buf_mb_len = 1;
	} else if (buf_mb_len == 0)
	    buf_mb_len++;

	/* Save the multibyte character in chr. */
	if (chr != NULL) {
	    int i;

	    for (i = 0; i < buf_mb_len; i++)
		chr[i] = buf[i];
	}

	/* Save the column width of the wide character in col. */
	if (col != NULL) {
	    /* If we have a tab, get its width in columns using the
	     * current value of col. */
	    if (*buf == '\t')
		*col += tabsize - *col % tabsize;
	    /* If we have a control character, get its width using one
	     * column for the "^" that will be displayed in front of it,
	     * and the width in columns of its visible equivalent as
	     * returned by control_mbrep(). */
	    else if (is_cntrl_mbchar(buf)) {
		char *ctrl_buf_mb = charalloc(MB_CUR_MAX);
		int ctrl_buf_mb_len;

		(*col)++;

		ctrl_buf_mb = control_mbrep(buf, ctrl_buf_mb,
			&ctrl_buf_mb_len);

		*col += mbwidth(ctrl_buf_mb);

		free(ctrl_buf_mb);
	    /* If we have a normal character, get its width in columns
	     * normally. */
	    } else
		*col += mbwidth(buf);
	}
    } else {
#endif
	/* Get the number of bytes in the byte character. */
	buf_mb_len = 1;

	/* Save the byte character in chr. */
	if (chr != NULL)
	    *chr = *buf;

	if (col != NULL) {
	    /* If we have a tab, get its width in columns using the
	     * current value of col. */
	    if (*buf == '\t')
		*col += tabsize - *col % tabsize;
	    /* If we have a control character, it's two columns wide:
	     * one column for the "^" that will be displayed in front of
	     * it, and one column for its visible equivalent as returned
	     * by control_mbrep(). */
	    else if (is_cntrl_char((unsigned char)*buf))
		*col += 2;
	    /* If we have a normal character, it's one column wide. */
	    else
		(*col)++;
	}
#ifdef ENABLE_UTF8
    }
#endif

    return buf_mb_len;
}

/* Return the index in buf of the beginning of the multibyte character
 * before the one at pos. */
size_t move_mbleft(const char *buf, size_t pos)
{
    size_t pos_prev = pos;

    assert(buf != NULL && pos <= strlen(buf));

    /* There is no library function to move backward one multibyte
     * character.  Here is the naive, O(pos) way to do it. */
    while (TRUE) {
	int buf_mb_len = parse_mbchar(buf + pos - pos_prev, NULL, NULL);

	if (pos_prev <= (size_t)buf_mb_len)
	    break;

	pos_prev -= buf_mb_len;
    }

    return pos - pos_prev;
}

/* Return the index in buf of the beginning of the multibyte character
 * after the one at pos. */
size_t move_mbright(const char *buf, size_t pos)
{
    return pos + parse_mbchar(buf + pos, NULL, NULL);
}

#ifndef HAVE_STRCASECMP
/* This function is equivalent to strcasecmp(). */
int nstrcasecmp(const char *s1, const char *s2)
{
    return strncasecmp(s1, s2, (size_t)-1);
}
#endif

/* This function is equivalent to strcasecmp() for multibyte strings. */
int mbstrcasecmp(const char *s1, const char *s2)
{
    return mbstrncasecmp(s1, s2, (size_t)-1);
}

#ifndef HAVE_STRNCASECMP
/* This function is equivalent to strncasecmp(). */
int nstrncasecmp(const char *s1, const char *s2, size_t n)
{
    assert(s1 != NULL && s2 != NULL);

    for (; n > 0 && *s1 != '\0' && *s2 != '\0'; n--, s1++, s2++) {
	if (tolower(*s1) != tolower(*s2))
	    break;
    }

    if (n > 0)
	return tolower(*s1) - tolower(*s2);
    else
	return 0;
}
#endif

/* This function is equivalent to strncasecmp() for multibyte
 * strings. */
int mbstrncasecmp(const char *s1, const char *s2, size_t n)
{
#ifdef ENABLE_UTF8
    if (ISSET(USE_UTF8)) {
	char *s1_mb = charalloc(MB_CUR_MAX);
	char *s2_mb = charalloc(MB_CUR_MAX);
	wchar_t ws1, ws2;

	assert(s1 != NULL && s2 != NULL);

	while (n > 0 && *s1 != '\0' && *s2 != '\0') {
	    bool bad_s1_mb = FALSE, bad_s2_mb = FALSE;
	    int s1_mb_len, s2_mb_len;

	    s1_mb_len = parse_mbchar(s1, s1_mb, NULL);

	    if (mbtowc(&ws1, s1_mb, s1_mb_len) < 0) {
		mbtowc(NULL, NULL, 0);
		ws1 = (unsigned char)*s1_mb;
		bad_s1_mb = TRUE;
	    }

	    s2_mb_len = parse_mbchar(s2, s2_mb, NULL);

	    if (mbtowc(&ws2, s2_mb, s2_mb_len) < 0) {
		mbtowc(NULL, NULL, 0);
		ws2 = (unsigned char)*s2_mb;
		bad_s2_mb = TRUE;
	    }

	    if (n == 0 || bad_s1_mb != bad_s2_mb ||
		towlower(ws1) != towlower(ws2))
		break;

	    s1 += s1_mb_len;
	    s2 += s2_mb_len;
	    n--;
	}

	free(s1_mb);
	free(s2_mb);

	return towlower(ws1) - towlower(ws2);
    } else
#endif
	return strncasecmp(s1, s2, n);
}

#ifndef HAVE_STRCASESTR
/* This function is equivalent to strcasestr().  It was adapted from
 * mutt's mutt_stristr() function. */
const char *nstrcasestr(const char *haystack, const char *needle)
{
    assert(haystack != NULL && needle != NULL);

    for (; *haystack != '\0'; haystack++) {
	const char *r = haystack, *q = needle;

	for (; tolower(*r) == tolower(*q) && *q != '\0'; r++, q++)
	    ;

	if (*q == '\0')
	    return haystack;
    }

    return NULL;
}
#endif

/* This function is equivalent to strcasestr() for multibyte strings. */
const char *mbstrcasestr(const char *haystack, const char *needle)
{
#ifdef ENABLE_UTF8
    if (ISSET(USE_UTF8)) {
	char *r_mb = charalloc(MB_CUR_MAX);
	char *q_mb = charalloc(MB_CUR_MAX);
	wchar_t wr, wq;
	bool found_needle = FALSE;

	assert(haystack != NULL && needle != NULL);

	while (*haystack != '\0') {
	    const char *r = haystack, *q = needle;
	    int r_mb_len, q_mb_len;

	    while (*q != '\0') {
		bool bad_r_mb = FALSE, bad_q_mb = FALSE;

		r_mb_len = parse_mbchar(r, r_mb, NULL);

		if (mbtowc(&wr, r_mb, r_mb_len) < 0) {
		    mbtowc(NULL, NULL, 0);
		    wr = (unsigned char)*r;
		    bad_r_mb = TRUE;
		}

		q_mb_len = parse_mbchar(q, q_mb, NULL);

		if (mbtowc(&wq, q_mb, q_mb_len) < 0) {
		    mbtowc(NULL, NULL, 0);
		    wq = (unsigned char)*q;
		    bad_q_mb = TRUE;
		}

		if (bad_r_mb != bad_q_mb ||
			towlower(wr) != towlower(wq))
		    break;

		r += r_mb_len;
		q += q_mb_len;
	    }

	    if (*q == '\0') {
		found_needle = TRUE;
		break;
	    }

	    haystack += move_mbright(haystack, 0);
	}

	free(r_mb);
	free(q_mb);

	return found_needle ? haystack : NULL;
    } else
#endif
	return strcasestr(haystack, needle);
}

#if !defined(NANO_SMALL) || !defined(DISABLE_TABCOMP)
/* This function is equivalent to strstr(), except in that it scans the
 * string in reverse, starting at rev_start. */
const char *revstrstr(const char *haystack, const char *needle, const
	char *rev_start)
{
    assert(haystack != NULL && needle != NULL && rev_start != NULL);

    for (; rev_start >= haystack; rev_start--) {
	const char *r, *q;

	for (r = rev_start, q = needle; *r == *q && *q != '\0'; r++, q++)
	    ;

	if (*q == '\0')
	    return rev_start;
    }

    return NULL;
}
#endif /* !NANO_SMALL || !DISABLE_TABCOMP */

#ifndef NANO_SMALL
/* This function is equivalent to strcasestr(), except in that it scans
 * the string in reverse, starting at rev_start. */
const char *revstrcasestr(const char *haystack, const char *needle,
	const char *rev_start)
{
    assert(haystack != NULL && needle != NULL && rev_start != NULL);

    for (; rev_start >= haystack; rev_start--) {
	const char *r = rev_start, *q = needle;

	for (; tolower(*r) == tolower(*q) && *q != '\0'; r++, q++)
	    ;

	if (*q == '\0')
	    return rev_start;
    }

    return NULL;
}

/* This function is equivalent to strcasestr() for multibyte strings,
 * except in that it scans the string in reverse, starting at
 * rev_start. */
const char *mbrevstrcasestr(const char *haystack, const char *needle,
	const char *rev_start)
{
#ifdef ENABLE_UTF8
    if (ISSET(USE_UTF8)) {
	char *r_mb = charalloc(MB_CUR_MAX);
	char *q_mb = charalloc(MB_CUR_MAX);
	wchar_t wr, wq;
	bool begin_line = FALSE, found_needle = FALSE;

	assert(haystack != NULL && needle != NULL && rev_start != NULL);

	while (!begin_line) {
	    const char *r = rev_start, *q = needle;
	    int r_mb_len, q_mb_len;

	    while (*q != '\0') {
		bool bad_r_mb = FALSE, bad_q_mb = FALSE;

		r_mb_len = parse_mbchar(r, r_mb, NULL);

		if (mbtowc(&wr, r_mb, r_mb_len) < 0) {
		    mbtowc(NULL, NULL, 0);
		    wr = (unsigned char)*r;
		    bad_r_mb = TRUE;
		}

		q_mb_len = parse_mbchar(q, q_mb, NULL);

		if (mbtowc(&wq, q_mb, q_mb_len) < 0) {
		    mbtowc(NULL, NULL, 0);
		    wq = (unsigned char)*q;
		    bad_q_mb = TRUE;
		}

		if (bad_r_mb != bad_q_mb ||
			towlower(wr) != towlower(wq))
		    break;

		r += r_mb_len;
		q += q_mb_len;
	    }

	    if (*q == '\0') {
		found_needle = TRUE;
		break;
	    }

	    if (rev_start == haystack)
		begin_line = TRUE;
	    else
		rev_start = haystack + move_mbleft(haystack, rev_start -
			haystack);
	}

	free(r_mb);
	free(q_mb);

	return found_needle ? rev_start : NULL;
    } else
#endif
	return revstrcasestr(haystack, needle, rev_start);
}
#endif /* !NANO_SMALL */

/* This function is equivalent to strlen() for multibyte strings. */
size_t mbstrlen(const char *s)
{
    return mbstrnlen(s, (size_t)-1);
}

#ifndef HAVE_STRNLEN
/* This function is equivalent to strnlen(). */
size_t nstrnlen(const char *s, size_t maxlen)
{
    size_t n = 0;

    assert(s != NULL);

    for (; maxlen > 0 && *s != '\0'; maxlen--, n++, s++)
	;

    return n;
}
#endif

/* This function is equivalent to strnlen() for multibyte strings. */
size_t mbstrnlen(const char *s, size_t maxlen)
{
    assert(s != NULL);

#ifdef ENABLE_UTF8
    if (ISSET(USE_UTF8)) {
	size_t n = 0;
	int s_mb_len;

	while (*s != '\0') {
	    s_mb_len = parse_mbchar(s, NULL, NULL);

	    if (maxlen == 0)
		break;

	    maxlen--;
	    s += s_mb_len;
	    n++;
	}

	return n;
    } else
#endif
	return strnlen(s, maxlen);
}

#ifndef DISABLE_JUSTIFY
/* This function is equivalent to strchr() for multibyte strings. */
char *mbstrchr(const char *s, char *c)
{
    assert(s != NULL && c != NULL);

#ifdef ENABLE_UTF8
    if (ISSET(USE_UTF8)) {
	bool bad_c_mb = FALSE, bad_s_mb = FALSE;
	char *s_mb = charalloc(MB_CUR_MAX);
	const char *q = s;
	wchar_t ws, wc;
	int c_mb_len = mbtowc(&wc, c, MB_CUR_MAX);

	if (c_mb_len < 0) {
	    mbtowc(NULL, NULL, 0);
	    wc = (unsigned char)*c;
	    bad_c_mb = TRUE;
	}

	while (*s != '\0') {
	    int s_mb_len = parse_mbchar(s, s_mb, NULL);

	    if (mbtowc(&ws, s_mb, s_mb_len) < 0) {
		mbtowc(NULL, NULL, 0);
		ws = (unsigned char)*s;
		bad_s_mb = TRUE;
	    }

	    if (bad_s_mb == bad_c_mb && ws == wc)
		break;

	    s += s_mb_len;
	    q += s_mb_len;
	}

	free(s_mb);

	if (ws != wc)
	    q = NULL;

	return (char *)q;
    } else
#endif
	return strchr(s, *c);
}

#ifdef ENABLE_NANORC
/* Return TRUE if the string s contains one or more blank characters,
 * and FALSE otherwise. */
bool has_blank_chars(const char *s)
{
    assert(s != NULL);

    for (; *s != '\0'; s++) {
	if (isblank(*s))
	    return TRUE;
    }

    return FALSE;
}

/* Return TRUE if the multibyte string s contains one or more blank
 * multibyte characters, and FALSE otherwise. */
bool has_blank_mbchars(const char *s)
{
    assert(s != NULL);

#ifdef ENABLE_UTF8
    if (ISSET(USE_UTF8)) {
	char *chr_mb = charalloc(MB_CUR_MAX);
	bool retval = FALSE;

	while (*s != '\0') {
	    int chr_mb_len;

	    chr_mb_len = parse_mbchar(s, chr_mb, NULL);

	    if (is_blank_mbchar(chr_mb)) {
		retval = TRUE;
		break;
	    }

	    s += chr_mb_len;
	}

	free(chr_mb);

	return retval;
    } else
#endif
	return has_blank_chars(s);
}
#endif /* ENABLE_NANORC */
#endif /* !DISABLE_JUSTIFY */

#ifdef ENABLE_UTF8
/* Return TRUE if wc is valid Unicode, and FALSE otherwise. */
bool is_valid_unicode(wchar_t wc)
{
    return ((0 <= wc && wc <= 0x10FFFF) && (wc <= 0xD7FF || 0xE000 <=
	wc) && (wc <= 0xFDCF || 0xFDF0 <= wc) && ((wc & 0xFFFF) <=
	0xFFFD));
}
#endif

#ifdef ENABLE_NANORC
/* Check if the string s is a valid multibyte string.  Return TRUE if it
 * is, and FALSE otherwise. */
bool is_valid_mbstring(const char *s)
{
    assert(s != NULL);

    return 
#ifdef ENABLE_UTF8
	ISSET(USE_UTF8) ?
	(mbstowcs(NULL, s, 0) != (size_t)-1) :
#endif

	TRUE;
}
#endif /* ENABLE_NANORC */

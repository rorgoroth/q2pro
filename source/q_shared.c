/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include <config.h>
#include "q_shared.h"

vec3_t vec3_origin = { 0, 0, 0 };

#if USE_CLIENT
const color_t colorBlack    = {   0,   0,   0, 255 };
const color_t colorRed      = { 255,   0,   0, 255 };
const color_t colorGreen    = {   0, 255,   0, 255 };
const color_t colorYellow   = { 255, 255,   0, 255 };
const color_t colorBlue     = {   0,   0, 255, 255 };
const color_t colorCyan     = {   0, 255, 255, 255 };
const color_t colorMagenta  = { 255,   0, 255, 255 };
const color_t colorWhite    = { 255, 255, 255, 255 };

const color_t colorTable[8] = {
    {   0,   0,   0, 255 },
    { 255,   0,   0, 255 },
    {   0, 255,   0, 255 },
    { 255, 255,   0, 255 },
    {   0,   0, 255, 255 },
    {   0, 255, 255, 255 },
    { 255,   0, 255, 255 },
    { 255, 255, 255, 255 }
};

const char colorNames[10][8] = {
    "black", "red", "green", "yellow",
    "blue", "cyan", "magenta", "white",
    "alt", "none"
};
#endif // USE_CLIENT

const vec3_t bytedirs[NUMVERTEXNORMALS] = {
#include "anorms.h"
};

int DirToByte( const vec3_t dir ) {
    int     i, best;
    float   d, bestd;
    
    if( !dir ) {
        return 0;
    }

    bestd = 0;
    best = 0;
    for( i = 0; i < NUMVERTEXNORMALS; i++ ) {
        d = DotProduct( dir, bytedirs[i] );
        if( d > bestd ) {
            bestd = d;
            best = i;
        }
    }
    
    return best;
}

void ByteToDir( int index, vec3_t dir ) {
    if( index < 0 || index >= NUMVERTEXNORMALS ) {
        Com_Error( ERR_FATAL, "ByteToDir: illegal index" );
    }

    VectorCopy( bytedirs[index], dir );
}

//============================================================================

void AngleVectors (vec3_t angles, vec3_t forward, vec3_t right, vec3_t up)
{
    float        angle;
    float        sr, sp, sy, cr, cp, cy;

    angle = angles[YAW] * (M_PI*2 / 360);
    sy = sin(angle);
    cy = cos(angle);
    angle = angles[PITCH] * (M_PI*2 / 360);
    sp = sin(angle);
    cp = cos(angle);
    angle = angles[ROLL] * (M_PI*2 / 360);
    sr = sin(angle);
    cr = cos(angle);

    if (forward)
    {
        forward[0] = cp*cy;
        forward[1] = cp*sy;
        forward[2] = -sp;
    }
    if (right)
    {
        right[0] = (-1*sr*sp*cy+-1*cr*-sy);
        right[1] = (-1*sr*sp*sy+-1*cr*cy);
        right[2] = -1*sr*cp;
    }
    if (up)
    {
        up[0] = (cr*sp*cy+-sr*-sy);
        up[1] = (cr*sp*sy+-sr*cy);
        up[2] = cr*cp;
    }
}


void ProjectPointOnPlane( vec3_t dst, const vec3_t p, const vec3_t normal )
{
    float d;
    vec3_t n;
    float inv_denom;

    inv_denom = 1.0F / DotProduct( normal, normal );

    d = DotProduct( normal, p ) * inv_denom;

    n[0] = normal[0] * inv_denom;
    n[1] = normal[1] * inv_denom;
    n[2] = normal[2] * inv_denom;

    dst[0] = p[0] - d * n[0];
    dst[1] = p[1] - d * n[1];
    dst[2] = p[2] - d * n[2];
}

/*
** assumes "src" is normalized
*/
void PerpendicularVector( vec3_t dst, const vec3_t src )
{
    int pos;
    int i;
    float minelem = 1.0F;
    vec3_t tempvec;

    /*
    ** find the smallest magnitude axially aligned vector
    */
    for ( pos = 0, i = 0; i < 3; i++ )
    {
        if ( fabs( src[i] ) < minelem )
        {
            pos = i;
            minelem = fabs( src[i] );
        }
    }
    tempvec[0] = tempvec[1] = tempvec[2] = 0.0F;
    tempvec[pos] = 1.0F;

    /*
    ** project the point onto the plane defined by src
    */
    ProjectPointOnPlane( dst, tempvec, src );

    /*
    ** normalize the result
    */
    VectorNormalize( dst );
}



//============================================================================

/*
==================
BoxOnPlaneSide

Returns 1, 2, or 1 + 2
==================
*/
#if !USE_ASM
int BoxOnPlaneSide( vec3_t emins, vec3_t emaxs, cplane_t *p )
{
    float   dist1, dist2;
    int     sides;
    
// general case
    switch (p->signbits)
    {
    case 0:
dist1 = p->normal[0]*emaxs[0] + p->normal[1]*emaxs[1] + p->normal[2]*emaxs[2];
dist2 = p->normal[0]*emins[0] + p->normal[1]*emins[1] + p->normal[2]*emins[2];
        break;
    case 1:
dist1 = p->normal[0]*emins[0] + p->normal[1]*emaxs[1] + p->normal[2]*emaxs[2];
dist2 = p->normal[0]*emaxs[0] + p->normal[1]*emins[1] + p->normal[2]*emins[2];
        break;
    case 2:
dist1 = p->normal[0]*emaxs[0] + p->normal[1]*emins[1] + p->normal[2]*emaxs[2];
dist2 = p->normal[0]*emins[0] + p->normal[1]*emaxs[1] + p->normal[2]*emins[2];
        break;
    case 3:
dist1 = p->normal[0]*emins[0] + p->normal[1]*emins[1] + p->normal[2]*emaxs[2];
dist2 = p->normal[0]*emaxs[0] + p->normal[1]*emaxs[1] + p->normal[2]*emins[2];
        break;
    case 4:
dist1 = p->normal[0]*emaxs[0] + p->normal[1]*emaxs[1] + p->normal[2]*emins[2];
dist2 = p->normal[0]*emins[0] + p->normal[1]*emins[1] + p->normal[2]*emaxs[2];
        break;
    case 5:
dist1 = p->normal[0]*emins[0] + p->normal[1]*emaxs[1] + p->normal[2]*emins[2];
dist2 = p->normal[0]*emaxs[0] + p->normal[1]*emins[1] + p->normal[2]*emaxs[2];
        break;
    case 6:
dist1 = p->normal[0]*emaxs[0] + p->normal[1]*emins[1] + p->normal[2]*emins[2];
dist2 = p->normal[0]*emins[0] + p->normal[1]*emaxs[1] + p->normal[2]*emaxs[2];
        break;
    case 7:
dist1 = p->normal[0]*emins[0] + p->normal[1]*emins[1] + p->normal[2]*emins[2];
dist2 = p->normal[0]*emaxs[0] + p->normal[1]*emaxs[1] + p->normal[2]*emaxs[2];
        break;
    default:
        dist1 = dist2 = 0;        // shut up compiler
        break;
    }

    sides = 0;
    if (dist1 >= p->dist)
        sides = 1;
    if (dist2 < p->dist)
        sides |= 2;

    return sides;
}
#endif // USE_ASM

vec_t VectorNormalize (vec3_t v)
{
    float    length, ilength;

    length = v[0]*v[0] + v[1]*v[1] + v[2]*v[2];
    length = sqrt (length);        // FIXME

    if (length)
    {
        ilength = 1/length;
        v[0] *= ilength;
        v[1] *= ilength;
        v[2] *= ilength;
    }
        
    return length;

}

vec_t VectorNormalize2 (vec3_t v, vec3_t out)
{
    float    length, ilength;

    length = v[0]*v[0] + v[1]*v[1] + v[2]*v[2];
    length = sqrt (length);        // FIXME

    if (length)
    {
        ilength = 1/length;
        out[0] = v[0]*ilength;
        out[1] = v[1]*ilength;
        out[2] = v[2]*ilength;
    }
        
    return length;

}

void ClearBounds (vec3_t mins, vec3_t maxs) {
    mins[0] = mins[1] = mins[2] = 99999;
    maxs[0] = maxs[1] = maxs[2] = -99999;
}

void AddPointToBounds (const vec3_t v, vec3_t mins, vec3_t maxs) {
    int        i;
    vec_t    val;

    for (i=0 ; i<3 ; i++)
    {
        val = v[i];
        if (val < mins[i])
            mins[i] = val;
        if (val > maxs[i])
            maxs[i] = val;
    }
}

void UnionBounds( vec3_t a[2], vec3_t b[2], vec3_t c[2] ) {
    c[0][0] = b[0][0] < a[0][0] ? b[0][0] : a[0][0];
    c[0][1] = b[0][1] < a[0][1] ? b[0][1] : a[0][1];
    c[0][2] = b[0][2] < a[0][2] ? b[0][2] : a[0][2];

    c[1][0] = b[1][0] > a[1][0] ? b[1][0] : a[1][0];
    c[1][1] = b[1][1] > a[1][1] ? b[1][1] : a[1][1];
    c[1][2] = b[1][2] > a[1][2] ? b[1][2] : a[1][2];
}

/*
=================
RadiusFromBounds
=================
*/
vec_t RadiusFromBounds (const vec3_t mins, const vec3_t maxs) {
    int     i;
    vec3_t  corner;
    vec_t   a, b;

    for (i=0 ; i<3 ; i++)
    {
        a = Q_fabs(mins[i]);
        b = Q_fabs(maxs[i]);
        corner[i] = a > b ? a : b;
    }

    return VectorLength (corner);
}

/*
===============
Q_CeilPowerOfTwo
===============
*/
int Q_CeilPowerOfTwo( int value ) {
    int i;

    for( i = 1; i < value; i <<= 1 )
        ;

    return i;
}


/*
====================
Com_CalcFov
====================
*/
float Com_CalcFov( float fov_x, float width, float height ) {
    float    a;
    float    x;

    if( fov_x < 1 || fov_x > 179 )
        Com_Error( ERR_DROP, "%s: bad fov: %f", __func__, fov_x );

    x = width / tan( fov_x / 360 * M_PI );

    a = atan( height / x );
    a = a * 360/ M_PI;

    return a;
}

void SetPlaneType( cplane_t *plane ) {
    vec_t *normal = plane->normal;
    
    if( normal[0] == 1 ) {
        plane->type = PLANE_X;
        return;
    }
    if( normal[1] == 1 ) {
        plane->type = PLANE_Y;
        return;
    }
    if( normal[2] == 1 ) {
        plane->type = PLANE_Z;
        return;
    }

    plane->type = PLANE_NON_AXIAL;
}

void SetPlaneSignbits( cplane_t *plane ) {
    int bits = 0;
    
    if( plane->normal[0] < 0 ) {
        bits |= 1;
    }
    if( plane->normal[1] < 0 ) {
        bits |= 2;
    }
    if( plane->normal[2] < 0 ) {
        bits |= 4;
    }
    plane->signbits = bits;
}



//====================================================================================

static const char hexchars[] = "0123456789ABCDEF";

/*
============
COM_SkipPath
============
*/
char *COM_SkipPath( const char *pathname ) {
    char    *last;

    if( !pathname ) {
        Com_Error( ERR_FATAL, "COM_SkipPath: NULL" );
    }
    
    last = (char *)pathname;
    while( *pathname ) {
        if( *pathname == '/' )
            last = (char *)pathname + 1;
        pathname++;
    }
    return last;
}

/*
============
COM_StripExtension
============
*/
void COM_StripExtension( const char *in, char *out, size_t size ) {
    char *s;

    Q_strlcpy( out, in, size );

    s = out + strlen( out );
    
    while( s != out ) {
        if( *s == '/' ) {
            break;
        }
        if( *s == '.' ) {
            *s = 0;
            break;
        }
        s--;
    }
}

/*
============
COM_FileExtension
============
*/
char *COM_FileExtension( const char *in ) {
    const char *s;
    const char *last;

    if( !in ) {
        Com_Error( ERR_FATAL, "%s: NULL", __func__ );
    }

    s = in + strlen( in );
    last = s;
    
    while( s != in ) {
        if( *s == '/' ) {
            break;
        }
        if( *s == '.' ) {
            return (char *)s;
        }
        s--;
    }

    return (char *)last;
}

/*
============
COM_FileBase
============
*/
void COM_FileBase (char *in, char *out)
{
    char *s, *s2;
    
    s = in + strlen(in) - 1;
    
    while (s != in && *s != '.')
        s--;
    
    for (s2 = s ; s2 != in && *s2 != '/' ; s2--)
    ;
    
    if (s-s2 < 2)
        out[0] = 0;
    else
    {
        s--;
        strncpy (out,s2+1, s-s2);
        out[s-s2] = 0;
    }
}

/*
============
COM_FilePath

Returns the path up to, but not including the last /
============
*/
void COM_FilePath( const char *in, char *out, size_t size ) {
    char *s;

    Q_strlcpy( out, in, size );
    s = strrchr( out, '/' );
    if( s ) {
        *s = 0;
    } else {
        *out = 0;
    }
}


/*
==================
COM_DefaultExtension

if path doesn't have .EXT, append extension
(extension should include the .)
==================
*/
size_t COM_DefaultExtension( char *path, const char *ext, size_t size ) {
    char    *src;
    size_t  len;

    if( *path ) {
        len = strlen( path );
        src = path + len - 1;

        while( *src != '/' && src != path ) {
            if( *src == '.' )
                return len;                 // it has an extension
            src--;
        }
    }

    len = Q_strlcat( path, ext, size );
    return len;
}

/*
==================
COM_IsFloat

Returns true if the given string is valid representation
of floating point number.
==================
*/
qboolean COM_IsFloat( const char *s ) {
    int c, dot = '.';

    if( *s == '-' ) {
        s++;
    }
    if( !*s ) {
        return qfalse;
    }

    do {
        c = *s++;
        if( c == dot ) {
            dot = 0;
        } else if( !Q_isdigit( c ) ) {
            return qfalse;
        }
    } while( *s );

    return qtrue;
}

qboolean COM_IsUint( const char *s ) {
    int c;

    if( !*s ) {
        return qfalse;
    }

    do {
        c = *s++;
        if( !Q_isdigit( c ) ) {
            return qfalse;
        }
    } while( *s );

    return qtrue;
}

qboolean COM_HasSpaces( const char *s ) {
    while( *s ) {
        if( *s <= 32 ) {
            return qtrue;
        }
        s++;
    }
    return qfalse;
}

unsigned COM_ParseHex( const char *s ) {
    int c;
    unsigned result;

    for( result = 0; *s; s++ ) {
        if( ( c = Q_charhex( *s ) ) == -1 ) {
            break;
        }
        if( result & ~( UINT_MAX >> 4 ) ) {
            return UINT_MAX;
        }
        result = c | ( result << 4 );
    }

    return result;
}

#if USE_CLIENT
qboolean COM_ParseColor( const char *s, color_t color ) {
    int i;
    int c[8];

    if( *s == '#' ) {
        s++;
        for( i = 0; s[i]; i++ ) {
            c[i] = Q_charhex( s[i] );
            if( c[i] == -1 ) {
                return qfalse;
            }
        }
        switch( i ) {
        case 3:
            color[0] = c[0] | ( c[0] << 4 );
            color[1] = c[1] | ( c[1] << 4 );
            color[2] = c[2] | ( c[2] << 4 );
            color[3] = 255;
            break;
        case 6:
            color[0] = c[1] | ( c[0] << 4 );
            color[1] = c[3] | ( c[2] << 4 );
            color[2] = c[5] | ( c[4] << 4 );
            color[3] = 255;
            break;
        case 8:
            color[0] = c[1] | ( c[0] << 4 );
            color[1] = c[3] | ( c[2] << 4 );
            color[2] = c[5] | ( c[4] << 4 );
            color[3] = c[7] | ( c[6] << 4 );
            break;
        default:
            return qfalse;
        }
        return qtrue;
    } else {
        for( i = 0; i < 8; i++ ) {
            if( !strcmp( colorNames[i], s ) ) {
                *( uint32_t * )color = *( uint32_t * )colorTable[i];
                return qtrue;
            }
        }
        return qfalse;
    }
}
#endif // USE_CLIENT

/*
=================
SortStrcmp
=================
*/
int QDECL SortStrcmp( const void *p1, const void *p2 ) {
    const char *s1 = *( const char ** )p1;
    const char *s2 = *( const char ** )p2;

    return strcmp( s1, s2 );
}

int QDECL SortStricmp( const void *p1, const void *p2 ) {
    const char *s1 = *( const char ** )p1;
    const char *s2 = *( const char ** )p2;

    return Q_stricmp( s1, s2 );
}

/*
=================
Com_WildCmp

Wildcard compare.
Returns non-zero if matches, zero otherwise.
=================
*/
int Com_WildCmp( const char *filter, const char *string, qboolean ignoreCase ) {
    switch( *filter ) {
    case '\0':
        return !*string;

    case '*':
        return Com_WildCmp( filter + 1, string, ignoreCase ) || (*string && Com_WildCmp( filter, string + 1, ignoreCase ));

    case '?':
        return *string && Com_WildCmp( filter + 1, string + 1, ignoreCase );

    default:
        return ((*filter == *string) || (ignoreCase && (Q_toupper( *filter ) == Q_toupper( *string )))) && Com_WildCmp( filter + 1, string + 1, ignoreCase );
    }
}

/*
================
Com_HashString
================
*/
unsigned Com_HashString( const char *string, int hashSize ) {
    unsigned hash, c;

    hash = 0;
    while( *string ) {
        c = *string++;
        hash = 127 * hash + c;
    }

    hash = ( hash >> 20 ) ^ ( hash >> 10 ) ^ hash;
    return hash & ( hashSize - 1 );
}

/*
================
Com_HashPath
================
*/
unsigned Com_HashPath( const char *string, int hashSize ) {
    unsigned hash, c;

    hash = 0;
    while( *string ) {
        c = *string++;
        if( c == '\\' ) {
            c = '/';
        } else {
            c = Q_tolower( c );
        }
        hash = 127 * hash + c;
    }

    hash = ( hash >> 20 ) ^ ( hash >> 10 ) ^ hash;
    return hash & ( hashSize - 1 );
}

/*
================
Q_DrawStrlen
================
*/
int Q_DrawStrlen( const char *string ) {
    int length;

    length = 0;
    while( *string ) {
        if( Q_IsColorString( string ) ) {
            string++;
        } else {
            length++;
        }
        string++;
    }

    return length;
}

/*
================
Q_DrawStrlenTo
================
*/
int Q_DrawStrlenTo( const char *string, int maxChars ) {
    int length;

    if( maxChars < 1 ) {
        maxChars = MAX_STRING_CHARS;
    }

    length = 0;
    while( *string && maxChars-- ) {
        if( Q_IsColorString( string ) ) {
            string++;
        } else {
            length++;
        }
        string++;
    }

    return length;
}

/*
================
Q_ClearColorStr

Removes color escape codes, high-bit and unprintable characters.
Return number of characters written, not including the NULL character.
================
*/
int Q_ClearColorStr( char *out, const char *in, int bufsize ) {
    char *p, *m;
    int c;

    if( bufsize < 1 ) {
        Com_Error( ERR_FATAL, "%s: bad bufsize: %d", __func__, bufsize );
    }

    p = out;
    m = out + bufsize - 1;
    while( *in && p < m ) {
        if( Q_IsColorString( in ) ) {
            in += 2;
            continue;
        }
        c = *in++;
        c &= 127;
        if( Q_isprint( c ) ) {
            *p++ = c;
        }
    }

    *p = 0;

    return p - out;
}

/*
================
Q_ClearStr

Removes high-bit and unprintable characters.
Return number of characters written, not including the NULL character.
================
*/
int Q_ClearStr( char *out, const char *in, int bufsize ) {
    char *p, *m;
    int c;

    if( bufsize < 1 ) {
        Com_Error( ERR_FATAL, "%s: bad bufsize: %d", __func__, bufsize );
    }

    p = out;
    m = out + bufsize - 1;
    while( *in && p < m ) {
        c = *in++;
        c &= 127;
        if( Q_isprint( c ) ) {
            *p++ = c;
        }
    }

    *p = 0;

    return p - out;
}

int Q_HighlightStr( char *out, const char *in, int bufsize ) {
    char *p, *m;
    int c;

    if( bufsize < 1 ) {
        Com_Error( ERR_FATAL, "%s: bad bufsize: %d", __func__, bufsize );
    }

    p = out;
    m = out + bufsize - 1;
    while( *in && p < m ) {
        c = *in++;
        c |= 128;
        *p++ = c;
    }

    *p = 0;

    return p - out;
}

/*
================
Q_IsWhiteSpace
================
*/
qboolean Q_IsWhiteSpace( const char *string ) {
    while( *string ) {
        if( ( *string & 127 ) > 32 ) {
            return qfalse;
        }
        string++;
    }

    return qtrue;
}

/*
================
Q_FormatString

replaces some common escape codes and unprintable characters
================
*/
char *Q_FormatString( const char *string ) {
    static char buffer[MAX_STRING_CHARS];
    char    *dst;
    int        c;

    dst = buffer;
    while( *string ) {
        c = *string++;

        switch( c ) {
        case '\t':
            *dst++ = '\\';
            *dst++ = 't';
            break;
        case '\b':
            *dst++ = '\\';
            *dst++ = 'b';
            break;
        case '\r':
            *dst++ = '\\';
            *dst++ = 'r';
            break;
        case '\n':
            *dst++ = '\\';
            *dst++ = 'n';
            break;
        case '\\':
            *dst++ = '\\';
            *dst++ = '\\';
            break;
        case '\"':
            *dst++ = '\\';
            *dst++ = '\"';
            break;
        default:
            if( c < 32 || c >= 127 ) {
                *dst++ = '\\';
                *dst++ = 'x';
                *dst++ = hexchars[c >> 4];
                *dst++ = hexchars[c & 15];
            } else {
                *dst++ = c;
            }
            break;
        }

        if( dst - buffer >= MAX_STRING_CHARS - 4 ) {
            break;
        }
    }

    *dst = 0;

    return buffer;
}

#if 0

typedef enum {
    ESC_CHR = ( 1 << 0 ),
    ESC_CLR = ( 1 << 1 )
} escape_t;

/*
================
Q_UnescapeString
================
*/
size_t Q_UnescapeString( char *out, const char *in, size_t bufsize, escape_t flags ) {
    char    *dst, *last;
    int        c;

    if( bufsize < 1 ) {
        Com_Error( ERR_FATAL, "%s: bad bufsize: %d", __func__, bufsize );
    }

    p = out;
    m = out + bufsize - 1;
    while( *in && p < m ) {
        c = *in++;

        if( ( flags & ESC_CHR ) && c == '\\' ) {
            c = *in++;
            switch( c ) {
            case 0:
                goto breakOut;
            case 't':
                c = '\t';
                break;
            case 'b':
                c = '\b';
                break;
            case 'r':
                c = '\r';
                break;
            case 'n':
                c = '\n';
                break;
            case '\\':
                c = '\\';
                break;
            case 'x':
                if( ( c = Q_charhex( in[0] ) ) == -1 ) {
                    goto breakOut;
                }
                    result = c | ( r << 4 );
                }
                break;
            default:
                break;
            }

            *p++ = c;
        }
    }

    *dst = 0;

    return buffer;

}
#endif

/*
================
Com_LocalTime
================
*/
void Com_LocalTime( qtime_t *qtime ) {
    time_t clock;
    struct tm    *localTime;

    time( &clock );
    localTime = localtime( &clock );

    *qtime = *localTime;
}

/*
============
va

does a varargs printf into a temp buffer, so I don't need to have
varargs versions of all text functions.
FIXME: make this buffer size safe someday
============
*/
char *va( const char *format, ... ) {
    va_list         argptr;
    static char     buffers[2][0x2800];
    static int      index;

    index ^= 1;
    
    va_start( argptr, format );
    Q_vsnprintf( buffers[index], sizeof( buffers[0] ), format, argptr );
    va_end( argptr );

    return buffers[index];    
}


static char     com_token[4][MAX_TOKEN_CHARS];
static int      com_tokidx;

static char *COM_GetToken( void ) {
    return com_token[com_tokidx++ & 3];
}

/*
==============
COM_SimpleParse

Parse a token out of a string.
==============
*/
char *COM_SimpleParse( const char **data_p, int *length ) {
    int         c;
    int         len;
    const char  *data;
    char        *s = COM_GetToken();

    data = *data_p;
    len = 0;
    s[0] = 0;
    if( length ) {
        *length = 0;
    }
    
    if( !data ) {
        *data_p = NULL;
        return s;
    }
        
// skip whitespace
    while( ( c = *data ) <= ' ' ) {
        if( c == 0 ) {
            *data_p = NULL;
            return s;
        }
        data++;
    }
    
// parse a regular word
    do {
        if( len < MAX_TOKEN_CHARS - 1 ) {
            s[len++] = c;
        }
        data++;
        c = *data;
    } while( c > 32 );

    s[len] = 0;

    if( length ) {
        *length = len;
    }

    *data_p = data;
    return s;
}


/*
==============
COM_Parse

Parse a token out of a string.
Handles C and C++ comments.
==============
*/
char *COM_Parse( const char **data_p ) {
    int         c;
    int         len;
    const char  *data;
    char        *s = COM_GetToken();

    data = *data_p;
    len = 0;
    s[0] = 0;
    
    if( !data ) {
        *data_p = NULL;
        return s;
    }
        
// skip whitespace
skipwhite:
    while( ( c = *data ) <= ' ' ) {
        if( c == 0 ) {
            *data_p = NULL;
            return s;
        }
        data++;
    }
    
// skip // comments
    if( c == '/' && data[1] == '/' ) {
        data += 2;
        while( *data && *data != '\n' )
            data++;
        goto skipwhite;
    }

// skip /* */ comments
    if( c == '/' && data[1] == '*' ) {
        data += 2;
        while( *data ) {
            if( data[0] == '*' && data[1] == '/' ) {
                data += 2;
                break;
            }
            data++;
        }
        goto skipwhite;
    }

// handle quoted strings specially
    if( c == '\"' ) {
        data++;
        while( 1 ) {
            c = *data++;
            if( c == '\"' || !c ) {
                goto finish;
            }

            if( len < MAX_TOKEN_CHARS - 1 ) {
                s[len++] = c;
            }
        }
    }

// parse a regular word
    do {
        if( len < MAX_TOKEN_CHARS - 1 ) {
            s[len++] = c;
        }
        data++;
        c = *data;
    } while( c > 32 );

finish:
    s[len] = 0;

    *data_p = data;
    return s;
}

/*
==============
COM_Compress

Operates in place, removing excess whitespace and comments.
Non-contiguous line feeds are preserved.

Returns resulting data length.
==============
*/
int COM_Compress( char *data ) {
    int     c, n = 0;
    char    *s = data, *d = data;

    while( *s ) {
        // skip whitespace    
        if( *s <= ' ' ) {
            n = ' ';
            do {
                c = *s++;
                if( c == '\n' ) {
                    n = '\n';
                }
                if( !c ) {
                    goto finish;
                }
            } while( *s <= ' ' );
        }
        
        // skip // comments
        if( s[0] == '/' && s[1] == '/' ) {
            n = ' ';
            s += 2;
            while( *s && *s != '\n' ) {
                s++;
            }
            continue;
        }

        // skip /* */ comments
        if( s[0] == '/' && s[1] == '*' ) {
            n = ' ';
            s += 2;
            while( *s ) {
                if( s[0] == '*' && s[1] == '/' ) {
                    s += 2;
                    break;
                }
                s++;
            }
            continue;
        }

        // add whitespace character
        if( n ) {
            *d++ = n;
            n = 0;
        }

        // handle quoted strings specially
        if( *s == '\"' ) {
            s++;
            *d++ = '\"';
            do {
                c = *s++;
                if( !c ) {
                    goto finish;
                }
                *d++ = c;
            } while( c != '\"' );
            continue;
        }

        // handle line feed escape
        if( *s == '\\' && s[1] == '\n' ) {
            s += 2;
            continue;
        }

        // parse a regular word
        do {
            *d++ = *s++;
        } while( *s > ' ' );
    }

finish:
    *d = 0;

    return d - data;
}


/*
===============
Com_PageInMemory

===============
*/
int    paged_total;

void Com_PageInMemory (void *buffer, int size)
{
    int        i;

    for (i=size-1 ; i>0 ; i-=4096)
        paged_total += (( byte * )buffer)[i];
}



/*
============================================================================

                    LIBRARY REPLACEMENT FUNCTIONS

============================================================================
*/

int Q_strncasecmp( const char *s1, const char *s2, size_t n ) {
    int        c1, c2;
    
    do {
        c1 = *s1++;
        c2 = *s2++;

        if( !n-- )
            return 0;        /* strings are equal until end point */
        
        if( c1 != c2 ) {
            c1 = Q_tolower( c1 );
            c2 = Q_tolower( c2 );
            if( c1 < c2 )
                return -1;
            if( c1 > c2 )
                return 1;        /* strings not equal */
        }
    } while( c1 );
    
    return 0;        /* strings are equal */
}

int Q_strcasecmp( const char *s1, const char *s2 ) {
    int        c1, c2;
    
    do {
        c1 = *s1++;
        c2 = *s2++;
        
        if( c1 != c2 ) {
            c1 = Q_tolower( c1 );
            c2 = Q_tolower( c2 );
            if( c1 < c2 )
                return -1;
            if( c1 > c2 )
                return 1;        /* strings not equal */
        }
    } while( c1 );
    
    return 0;        /* strings are equal */
}

/*
===============
Q_strlcpy
===============
*/
size_t Q_strlcpy( char *dst, const char *src, size_t size ) {
    size_t ret = strlen( src );

    if( size ) {
        size_t len = ret >= size ? size - 1 : ret;
        memcpy( dst, src, len );
        dst[len] = 0;
    }

    return ret;
}

/*
===============
Q_strlcat
===============
*/
size_t Q_strlcat( char *dst, const char *src, size_t size ) {
    size_t srclen = strlen( src );
    size_t dstlen = strlen( dst );
    size_t ret = srclen + dstlen;

    if( dstlen >= size ) {
        Com_Error( ERR_FATAL, "%s: already overflowed", __func__ );
    }

    size -= dstlen;
    dst += dstlen;

    if( size ) {
        size_t len = srclen >= size ? size - 1 : srclen;
        memcpy( dst, src, len );
        dst[len] = 0;
    }

    return ret;
}

/*
===============
Q_concat
===============
*/
size_t Q_concat( char *dest, size_t destsize, ... ) {
    va_list argptr;
    const char *s;
    size_t len, total = 0;

    va_start( argptr, destsize );

    if( destsize ) {
        while( ( s = va_arg( argptr, const char * ) ) != NULL ) {
            len = strlen( s );
            if( total + len < destsize ) {
                memcpy( dest, s, len );
                dest += len;
            }
            total += len;
        }
        *dest = 0;
    } else {
        while( ( s = va_arg( argptr, const char * ) ) != NULL ) {
            len = strlen( s );
            total += len;
        }
    }

    va_end( argptr );

    return total;
}

/*
===============
Q_vsnprintf

Work around broken M$ CRT semantics.
===============
*/
size_t Q_vsnprintf( char *dest, size_t size, const char *fmt, va_list argptr ) {
    int ret;

#ifdef _WIN32
    if( !size ) {
        return 0;
    }
    ret = _vsnprintf( dest, size - 1, fmt, argptr );
    if( ret >= size - 1 ) {
        dest[size - 1] = 0;
    }
#else
    ret = vsnprintf( dest, size, fmt, argptr );
#endif

    if( ret < 0 ) {
        if( size ) {
            *dest = 0;
        }
        ret = 0;
    }

    return ret;
}

/*
===============
Q_snprintf
===============
*/
size_t Q_snprintf( char *dest, size_t size, const char *fmt, ... ) {
    va_list argptr;
    size_t  ret;

    va_start( argptr, fmt );
    ret = Q_vsnprintf( dest, size, fmt, argptr );
    va_end( argptr );

    return ret;
}

char *Q_strchrnul( const char *s, int c ) {
    while( *s ) {
        if( *s == c ) {
            return ( char * )s;
        }
        s++;
    }
    return ( char * )s;
}

void Q_setenv( const char *name, const char *value ) {
#ifdef _WIN32
    if( !value ) {
        value = "";
    }
#if( _MSC_VER >= 1400 )
    _putenv_s( name, value );
#else
    _putenv( va( "%s=%s", name, value ) );
#endif
#else // _WIN32
    if( value ) {
        setenv( name, value, 1 );
    } else {
        unsetenv( name );
    }
#endif // !_WIN32
}

/*
=====================================================================

  INFO STRINGS

=====================================================================
*/

/*
===============
Info_ValueForKey

Searches the string for the given
key and returns the associated value, or an empty string.
===============
*/
char *Info_ValueForKey( const char *s, const char *key ) {
    static char value[4][MAX_INFO_STRING];  // use 4 buffers so compares
                                            // work without stomping on each other
    static int  valueindex;
    char        pkey[MAX_INFO_STRING];
    char        *o;

    valueindex++;
    if( *s == '\\' )
        s++;
    while( 1 ) {
        o = pkey;
        while( *s != '\\' ) {
            if( !*s )
                return "";
            *o++ = *s++;
        }
        *o = 0;
        s++;

        o = value[valueindex & 3];
        while( *s != '\\' && *s ) {
            *o++ = *s++;
        }
        *o = 0;

        if( !strcmp( key, pkey ) )
            return value[valueindex & 3];

        if( !*s )
            return "";
        s++;
    }

    return "";
}

/*
==================
Info_RemoveKey
==================
*/
void Info_RemoveKey( char *s, const char *key ) {
    char    *start;
    char    pkey[MAX_INFO_STRING];
    char    *o;

    while( 1 ) {
        start = s;
        if( *s == '\\' )
            s++;
        o = pkey;
        while( *s != '\\' ) {
            if( !*s )
                return;
            *o++ = *s++;
        }
        *o = 0;
        s++;

        while( *s != '\\' && *s ) {
            s++;
        }

        if( !strcmp( key, pkey ) ) {
            o = start; // remove this part
            while( *s ) {
                *o++ = *s++;
            }
            *o = 0;
            s = start;
            continue; // search for duplicates
        }

        if( !*s )
            return;
    }

}


/*
==================
Info_Validate

Some characters are illegal in info strings because they
can mess up the server's parsing.
Also checks the length of keys/values and the whole string.
==================
*/
qboolean Info_Validate( const char *s ) {
    const char  *start;
    int         c, len;
    
    start = s;
    while( 1 ) {
        //
        // validate key
        //
        if( *s == '\\' ) {
            s++;
        }
        if( !*s ) {
            return qfalse;    // missing key
        }
        len = 0;
        while( *s != '\\' ) {
            c = *s & 127;
            if( c == '\\' || c == '\"' || c == ';' ) {
                return qfalse;    // illegal characters
            }
            if( len == MAX_INFO_KEY - 1 ) {
                return qfalse;    // oversize key
            }
            s++; len++;
            if( !*s ) {
                return qfalse;    // missing value
            }
        }

        //
        // validate value
        //
        s++;
        if( !*s ) {
            return qfalse;    // missing value
        }
        len = 0;
        while( *s != '\\' ) {
            c = *s & 127;
            if( c == '\\' || c == '\"' || c == ';' ) {
                return qfalse;    // illegal characters
            }
            if( len == MAX_INFO_VALUE - 1 ) {
                return qfalse;    // oversize value
            }
            s++; len++;
            if( !*s ) {
                if( s - start > MAX_INFO_STRING ) {
                    return qfalse;
                }
                return qtrue;    // end of string
            }
        }
    }

    return qfalse; // quiet compiler warning
}

/*
============
Info_ValidateSubstring
============
*/
int Info_SubValidate( const char *s ) {
    const char *start;
    int c, len;

    for( start = s; *s; s++ ) {
        c = *s & 127;
        if( c == '\\' || c == '\"' || c == ';' ) {
            return -1;
        }
    }

    len = s - start;
    if( len >= MAX_QPATH ) {
        return -1;
    }

    return len;
}

/*
==================
Info_SetValueForKey
==================
*/
qboolean Info_SetValueForKey( char *s, const char *key, const char *value ) {
    char    newi[MAX_INFO_STRING], *v;
    int     c, l, kl, vl;

    // validate key
    kl = Info_SubValidate( key );
    if( kl == -1 ) {
        return qfalse;
    }

    // validate value
    vl = Info_SubValidate( value );
    if( vl == -1 ) {
        return qfalse;
    }

    Info_RemoveKey( s, key );
    if( !vl ) {
        return qtrue;
    }

    l = ( int )strlen( s );
    if( l + kl + vl + 2 >= MAX_INFO_STRING ) {
        return qfalse;
    }

    newi[0] = '\\';
    memcpy( newi + 1, key, kl );
    newi[kl + 1] = '\\';
    memcpy( newi + kl + 2, value, vl + 1 );

    // only copy ascii values
    s += l;
    v = newi;
    while( *v ) {
        c = *v++;
        c &= 127;        // strip high bits
        if( c >= 32 && c < 127 )
            *s++ = c;
    }
    *s = 0;

    return qtrue;
}

/*
==================
Info_NextPair
==================
*/
void Info_NextPair( const char **string, char *key, char *value ) {
    char        *o;
    const char  *s;

    *value = *key = 0;

    s = *string;
    if( !s ) {
        return;
    }

    if( *s == '\\' )
        s++;

    if( !*s ) {
        *string = NULL;
        return;
    }
    
    o = key;
    while( *s && *s != '\\' ) {
        *o++ = *s++;
    }
    
    *o = 0;

    if( !*s ) {
        *string = NULL;
        return;
    }

    o = value;
    s++;
    while( *s && *s != '\\' ) {
        *o++ = *s++;
    }
    *o = 0;

    if( *s ) {
        s++;
    }

    *string = s;
    
}

/*
==================
Info_Print
==================
*/
void Info_Print( const char *infostring ) {
    char    key[MAX_INFO_STRING];
    char    value[MAX_INFO_STRING];

    while( infostring ) {
        Info_NextPair( &infostring, key, value );
        
        if( !key[0] ) {
            break;
        }

        if( value[0] ) {
            Com_Printf( "%-20s %s\n", key, value );
        } else {
            Com_Printf( "%-20s <MISSING VALUE>\n", key );
        }
    }
}


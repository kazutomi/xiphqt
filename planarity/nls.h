#ifdef ENABLE_NLS
#include "gettext.h"
#include <locale.h>
#include <libintl.h>

#define GT_DOMAIN X_GT_DOMAIN(UGT_DOMAIN)
#define X_GT_DOMAIN(x) XX_GT_DOMAIN(x)
#define XX_GT_DOMAIN(x) #x

#define GT_DIR X_GT_DIR(UGT_DIR)
#define X_GT_DIR(x) XX_GT_DIR(x)
#define XX_GT_DIR(x) #x

#define _(x) gettext(x)
#else
#define _(x) x
#endif


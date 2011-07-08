#ifndef HYBRID_TOOLTIP_H
#define HYBRID_TOOLTIP_H
#include <gtk/gtk.h>

typedef struct _HybridTooltip HybridTooltip;
typedef struct _HybridTooltipData HybridTooltipData;

struct _HybridTooltip {
	GtkWidget *widget;
	GtkWidget *window;
	guint source;
};

struct _HybridTooltipData {
	GtkWidget *widget;
	gpointer user_data;
};

#ifdef __cplusplus
extern "C" {
#endif

void hybrid_tooltip_setup(GtkWidget *widget, gpointer user_data);

#ifdef __cplusplus
}
#endif

#endif /* HYBRID_TOOLTIP_H */
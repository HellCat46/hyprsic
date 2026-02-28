#include "window.hpp"
#include "manager.hpp"


BrightnessWindow::BrightnessWindow(AppContext *ctx, BrightnessManager* manager) : ctx(ctx), manager(manager){}

void BrightnessWindow::init() {
    winBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
  
    GtkWidget *titleLbl = gtk_label_new(nullptr);
    gtk_label_set_markup(GTK_LABEL(titleLbl), "<b>Brightness</b>");
    gtk_box_pack_start(GTK_BOX(winBox), titleLbl, FALSE, FALSE, 0);
  
    adjWid = gtk_adjustment_new(0, 0, 100, 5, 10, 0);
    GtkWidget *scale = gtk_scale_new(GTK_ORIENTATION_HORIZONTAL, adjWid);
    gtk_scale_set_value_pos(GTK_SCALE(scale), GTK_POS_TOP);
    gtk_box_pack_start(GTK_BOX(winBox), scale, TRUE, TRUE, 0);
    g_signal_connect(scale, "change-value",
                     G_CALLBACK(BrightnessWindow::handleScaleChange), this);
  
    gtk_widget_show_all(winBox);
    
    gtk_widget_set_margin_top(winBox, 20);
    gtk_widget_set_margin_bottom(winBox, 20);
    gtk_widget_set_margin_start(winBox, 10);
    gtk_widget_set_margin_end(winBox, 10);
    
    ctx->addModule(winBox, "brightness");
}

void BrightnessWindow::update() {
    short brightness = manager->getLvl();
    
    if (brightness >= 0) {
        gtk_adjustment_set_value(adjWid, brightness);
    }
}

void BrightnessWindow::handleScaleChange(GtkRange *range, GtkScrollType *scroll,
                                         gdouble value, gpointer data) {
  BrightnessWindow *self = static_cast<BrightnessWindow *>(data);

  if (self->manager->setLvl(static_cast<short>(value))) {
    self->update();
  }
}

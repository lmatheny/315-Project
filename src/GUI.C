#include <gtk/gtk.h>

// Function to apply CSS to widgets
static void apply_css(GtkWidget* widget, GtkStyleProvider* provider) {
    gtk_style_context_add_provider(
        gtk_widget_get_style_context(widget),
        provider,
        GTK_STYLE_PROVIDER_PRIORITY_USER
    );
}

static void show_calendar(GtkWidget* widget, gpointer data) {
    GtkWidget* window_calendar = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window_calendar), "Find a Date");
    gtk_window_set_default_size(GTK_WINDOW(window_calendar), 300, 350);

    GtkWidget* grid_calendar = gtk_grid_new();
    gtk_container_add(GTK_CONTAINER(window_calendar), grid_calendar);

    // Create a calendar widget
    GtkWidget* calendar = gtk_calendar_new();
    gtk_grid_attach(GTK_GRID(grid_calendar), calendar, 0, 1, 1, 1);

    // Customize the calendar further here (e.g., mark a day)
    // ...

    // Show all widgets
    gtk_widget_show_all(window_calendar);
}

int main(int argc, char* argv[]) {
    // Initialize GTK+
    gtk_init(&argc, &argv);

    // Create a new CSS provider and load our CSS data
    GtkCssProvider* css_provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(css_provider,
        "window {"
        "  background-color: #E6D395;" // Example background color, change as needed
        "}", -1, NULL);

    // Create the Main Window
    GtkWidget* window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Weatherly");
    gtk_window_set_default_size(GTK_WINDOW(window), 550, 400);

    // Connect the "destroy" event to the gtk_main_quit function
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    // Apply CSS to the window
    apply_css(window, GTK_STYLE_PROVIDER(css_provider));

    // Create a Grid Layout
    GtkWidget* grid = gtk_grid_new();
    gtk_container_add(GTK_CONTAINER(window), grid);

    // Create the button images
    GtkWidget* image1 = gtk_image_new_from_file("Images/calendar.png");
    GtkWidget* image2 = gtk_image_new_from_file("Images/graph.png");

    // Add buttons in the top corners with images
    GtkWidget* button1 = gtk_button_new();
    gtk_button_set_image(GTK_BUTTON(button1), image1);
    gtk_grid_attach(GTK_GRID(grid), button1, 0, 0, 1, 1);
    gtk_widget_set_halign(button1, GTK_ALIGN_START); // Align to start of window (left)
    gtk_widget_set_valign(button1, GTK_ALIGN_START); // Align to top of window
    g_signal_connect(button1, "clicked", G_CALLBACK(show_calendar), NULL);


    

    // Add a Label for the "Weatherly" title and center it
    GtkWidget* title_label = gtk_label_new("Weatherly");
    gtk_grid_attach(GTK_GRID(grid), title_label, 1, 1, 1, 1);
    gtk_widget_set_halign(title_label, GTK_ALIGN_CENTER);

    // Add an Entry for the zip code input and make it smaller
    GtkWidget* zip_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(zip_entry), "Zip Code");
    gtk_entry_set_max_width_chars(GTK_ENTRY(zip_entry), 10);
    gtk_grid_attach(GTK_GRID(grid), zip_entry, 1, 2, 1, 1);
    gtk_widget_set_halign(zip_entry, GTK_ALIGN_CENTER);

    // Add a Weather Icon and center it
    GtkWidget* weather_icon = gtk_image_new_from_file("Images/weatherly.png"); // Replace with your image path
    gtk_grid_attach(GTK_GRID(grid), weather_icon, 1, 3, 1, 1);
    gtk_widget_set_halign(weather_icon, GTK_ALIGN_CENTER);

    // Add a Label for Temperature and center it
    // Create a label with larger font using Pango markup
    GtkWidget* temperature_label = gtk_label_new(NULL);
    const char* markup = "<span font='20'>--\u00B0</span>"; // For a larger font with a degree symbol
    gtk_label_set_markup(GTK_LABEL(temperature_label), markup);
    gtk_grid_attach(GTK_GRID(grid), temperature_label, 1, 4, 1, 1);
    gtk_widget_set_halign(temperature_label, GTK_ALIGN_CENTER);

    // Show all widgets in the window
    gtk_widget_show_all(window);

    // Start the GTK+ main loop
    gtk_main();

    // Clean up the CSS provider
    g_object_unref(css_provider);

    return 0;
}

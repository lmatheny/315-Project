#include <gtk/gtk.h>
#include <string.h>
#include <curl/curl.h>
#include <jansson.h>

// Global variables to store GTK widgets
GtkWidget *weather_description_label;
GtkWidget *temperature_label;
GtkWidget *feels_like_label;
GtkWidget *uv_index_label;
GtkWidget *location_query_entry;
GtkWidget *notebook;
GtkWidget *print_history_button;
GtkWidget *logo_image;
GtkWidget *weather_image;

size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp);
void on_button_clicked(GtkWidget *widget, gpointer data);
double celsiusToFahrenheit(double celsius);

 
// Function to update the live temperature next to the city name
void update_city_temperature_label(GtkWidget *label, const char *city) {
    // Make an API call to retrieve live weather data for the given city
    char *encoded_city = curl_easy_escape(NULL, city, 0);

    char api_url[256];
    snprintf(api_url, sizeof(api_url), "http://api.weatherstack.com/current?access_key=753d81b2ef3d2506e56030d1971f3ddc&query=%s&units=m",
             encoded_city);

    CURL *curl = curl_easy_init();
    if (!curl) {
        fprintf(stderr, "Error initializing libcurl\n");
        curl_free(encoded_city);
        return;
    }

    curl_easy_setopt(curl, CURLOPT_URL, api_url);
    curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "");

    char response_data[4096];
    memset(response_data, 0, sizeof(response_data));

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, response_data);

    CURLcode res = curl_easy_perform(curl);

    if (res != CURLE_OK) {
        fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        curl_easy_cleanup(curl);
        curl_free(encoded_city);
        return;
    }

    curl_easy_cleanup(curl);
    curl_free(encoded_city);

    // Parse JSON data to retrieve live temperature
    json_t *root;
    json_error_t error;

    if (response_data[0] == '\0') {
        fprintf(stderr, "Empty response received from the API\n");
        return;
    }

    root = json_loads(response_data, JSON_DECODE_ANY | JSON_DISABLE_EOF_CHECK, &error);
    if (!root) {
        fprintf(stderr, "Error parsing JSON: %s\n", error.text);
        return;
    }

    json_t *current_object = json_object_get(root, "current");
    if (current_object) {
        json_t *temperature_object = json_object_get(current_object, "temperature");
        if (json_is_number(temperature_object)) {
            double live_temperature_celsius = json_number_value(temperature_object);

            // Convert Celsius to Fahrenheit
            double live_temperature_fahrenheit = celsiusToFahrenheit(live_temperature_celsius);

            // Update the label with the live temperature in Fahrenheit and city name
            char city_temperature_text[100];
            snprintf(city_temperature_text, sizeof(city_temperature_text), "%s: %.2f Fahrenheit", city, live_temperature_fahrenheit);

            // Update font size and spacing for the label
            PangoAttrList *attr_list = pango_attr_list_new();
            PangoAttribute *attr = pango_attr_size_new(17500); // Adjust the size as needed
            pango_attr_list_insert(attr_list, attr);

            gtk_label_set_text(GTK_LABEL(label), city_temperature_text);
            gtk_label_set_attributes(GTK_LABEL(label), attr_list);

            pango_attr_list_unref(attr_list);
        } else {
            fprintf(stderr, "Temperature not found or not a number in JSON\n");
        }
    } else {
        fprintf(stderr, "Current object not found in JSON\n");
    }

    json_decref(root);
}

GtkWidget *create_logo_image() {
    // Specify the path to the logo image
    const char *logo_filename = "./weatherly.png"; // Update the filename as needed

    // Load the logo image
    GdkPixbuf *logo_pixbuf = gdk_pixbuf_new_from_file(logo_filename, NULL);
    if (!logo_pixbuf) {
        g_printerr("Failed to load logo image: %s\n", logo_filename);
        return NULL;
    }

    // Create the logo image widget
    GtkWidget *logo = gtk_image_new_from_pixbuf(logo_pixbuf);

    // Free the allocated memory for the logo pixbuf
    g_object_unref(logo_pixbuf);

    return logo;
}


GtkWidget *create_world_page(const GdkRGBA *background_color) {
    GtkWidget *page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);

    // Add spacing above the title label
    GtkWidget *title_spacing_label = gtk_label_new("");
    gtk_box_pack_start(GTK_BOX(page), title_spacing_label, FALSE, FALSE, 3);

    // Add a title to the page
    GtkWidget *title_label = gtk_label_new("Live Temperatures Around the World");
    PangoAttrList *title_attr_list = pango_attr_list_new();

    // Uncomment the following line to make the title text bold
    PangoAttribute *title_attr_bold = pango_attr_weight_new(PANGO_WEIGHT_BOLD);
    pango_attr_list_insert(title_attr_list, title_attr_bold);

    PangoAttribute *title_attr_size = pango_attr_size_new(20000); // Adjust the size as needed
    pango_attr_list_insert(title_attr_list, title_attr_size);

    gtk_label_set_attributes(GTK_LABEL(title_label), title_attr_list);
    gtk_box_pack_start(GTK_BOX(page), title_label, FALSE, FALSE, 0);

    // Specify the background color for each city label
    GdkRGBA city_label_background_color = {0.9, 0.9, 0.9, 1.0};  // Adjust the RGBA values as needed

    // Create labels for major cities
    const char *cities[] = {"New York", "London", "Sydney", "Tokyo", "Paris", "Beijing", "Dubai", "Los Angeles"};

    // Create labels to display live temperatures for each city
    for (int i = 0; i < sizeof(cities) / sizeof(cities[0]); ++i) {
        // Create a box to hold each city label with a specified background color
        GtkWidget *city_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
        gtk_widget_override_background_color(city_box, GTK_STATE_FLAG_NORMAL, &city_label_background_color);

        // Create a label to display city name and live temperature
        GtkWidget *city_temperature_label = gtk_label_new("");

        // Update the label with the live temperature and city name
        update_city_temperature_label(city_temperature_label, cities[i]);

        // Pack the label into the box
        gtk_box_pack_start(GTK_BOX(city_box), city_temperature_label, FALSE, FALSE, 0);

        // Pack each city box into the page with additional spacing
        gtk_box_pack_start(GTK_BOX(page), city_box, FALSE, FALSE, 5);
    }

    // Set the background color of the page
    gtk_widget_override_background_color(page, GTK_STATE_FLAG_NORMAL, background_color);

    return page;
}


// Structure to represent weather data
typedef struct WeatherData {
    char city[50];
    double temperature;
    double feels_like;
    int uv_index;
    char weather_description[50];
    struct WeatherData *next;
} WeatherData;

// Head of the linked list to store weather history
WeatherData *weather_data_head = NULL;

// Function to write the linked list to a file
void write_linked_list_to_file(const char *filename) {
    FILE *file = fopen(filename, "w");
    if (file == NULL) {
        perror("Error opening file");
        return;
    }

    WeatherData *current = weather_data_head;
    while (current != NULL) {
        fprintf(file, "City: %s\n", current->city);
        fprintf(file, "Temperature: %.2f Fahrenheit\n", current->temperature);
        fprintf(file, "Feels Like: %.2f Fahrenheit\n", current->feels_like);
        fprintf(file, "UV Index: %d\n", current->uv_index);
        fprintf(file, "Weather Description: %s\n", current->weather_description);
        fprintf(file, "------------------------\n");

        current = current->next;
    }

    fclose(file);
}

// Function to print the weather history
void print_weather_history() {
    // Specify the filename for the history file
    const char *history_filename = "weather_history.txt";

    // Call the function to write the linked list to the history file
    write_linked_list_to_file(history_filename);

    // Optionally, display a message indicating that the history is written to the file
    g_print("Weather history has been written to the file: %s\n", history_filename);
}

// Callback function for the "Print History" button
void on_print_history_button_clicked(GtkWidget *widget, gpointer data) {
    // Specify the filename for the history file
    const char *history_filename = "weather_history.txt";

    // Call the function to write the linked list to the history file
    write_linked_list_to_file(history_filename);

    // Optionally, display a message indicating that the history is written to the file
    g_print("Weather history has been written to the file: %s\n", history_filename);
}

// Callback function for the "Write to File" button
void on_write_to_file_button_clicked(GtkWidget *widget, gpointer data) {
    // Specify the filename for the output file
    const char *output_filename = "weather_output.txt";

    // Call the function to write the linked list to the output file
    write_linked_list_to_file(output_filename);

    // Optionally, display a message indicating that the data is written to the file
    g_print("Weather data has been written to the file: %s\n", output_filename);
}

// Your existing code for API call and parsing JSON

// Function to free memory used by the linked list
void free_weather_data() {
    WeatherData *current = weather_data_head;
    while (current != NULL) {
        WeatherData *next = current->next;
        free(current);
        current = next;
    }
}

// Function to set the image based on weather description
void update_weather_image(const char *weather_description) {
    const char *image_filename;

    // Specify the path to the "Images" folder
    const char *images_folder = "./"; // Assuming images are in the same folder as the executable

    // Use a switch-case statement to check the weather description
    if (strcmp(weather_description, "Overcast") == 0) {
        image_filename = g_strdup_printf("%s%s", images_folder, "cloudy.png");
    } else if (strcmp(weather_description, "Sunny") == 0) {
        image_filename = g_strdup_printf("%s%s", images_folder, "sunny.png");
    } else if (strcmp(weather_description, "Snow") == 0) {
        image_filename = g_strdup_printf("%s%s", images_folder, "snow.png");
    } else if (strcmp(weather_description, "Thunderstorm") == 0) {
        image_filename = g_strdup_printf("%s%s", images_folder, "thunderstorm.png");
    } else {
        image_filename = g_strdup_printf("%s%s", images_folder, "default.png");
    }

    // Load the image and set it on the weather image widget
    GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file(image_filename, NULL);
    if (!pixbuf) {
        g_printerr("Failed to load image: %s\n", image_filename);
        g_free((gpointer)image_filename);
        return;
    }
    gtk_image_set_from_pixbuf(GTK_IMAGE(weather_image), pixbuf);

    // Free the allocated memory for the filename
    g_free((gpointer)image_filename);
}



size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    char *data = (char *)userp;
    memcpy(data, contents, realsize);
    return realsize;
}

double celsiusToFahrenheit(double celsius) {
    return (celsius * 9.0 / 5.0) + 32.0;
}

void update_labels(const char *city, double temperature, double feels_like, int uv_index,
                   const char *weather_description) {
    // Update GTK labels with the received data
    gtk_label_set_text(GTK_LABEL(weather_description_label), weather_description);

    char temperature_text[50];
    snprintf(temperature_text, sizeof(temperature_text), "Temperature: %.2f Fahrenheit", temperature);
    gtk_label_set_text(GTK_LABEL(temperature_label), temperature_text);

    char feels_like_text[50];
    snprintf(feels_like_text, sizeof(feels_like_text), "Feels Like: %.2f Fahrenheit", feels_like);
    gtk_label_set_text(GTK_LABEL(feels_like_label), feels_like_text);

    char uv_index_text[20];
    snprintf(uv_index_text, sizeof(uv_index_text), "UV Index: %d", uv_index);
    gtk_label_set_text(GTK_LABEL(uv_index_label), uv_index_text);

    // Call the function to update the weather image
    update_weather_image(weather_description);
}

void on_button_clicked(GtkWidget *widget, gpointer data) {
    // Retrieve location query from the text field
    const char *location_query = gtk_entry_get_text(GTK_ENTRY(location_query_entry));

    // Your existing code for making the API call and processing the response
    char *encoded_location_query = curl_easy_escape(NULL, location_query, 0);

    char api_url[256];
    snprintf(api_url, sizeof(api_url), "http://api.weatherstack.com/current?access_key=753d81b2ef3d2506e56030d1971f3ddc&query=%s&units=m",
             encoded_location_query);

    CURL *curl = curl_easy_init();
    if (!curl) {
        fprintf(stderr, "Error initializing libcurl\n");
        curl_free(encoded_location_query);
        return;
    }

    curl_easy_setopt(curl, CURLOPT_URL, api_url);
    curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "");

    char response_data[4096];
    memset(response_data, 0, sizeof(response_data));

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, response_data);

    CURLcode res = curl_easy_perform(curl);

    if (res != CURLE_OK) {
        fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        curl_easy_cleanup(curl);
        curl_free(encoded_location_query);
        return;
    }

    curl_easy_cleanup(curl);
    curl_free(encoded_location_query);

    // Parse JSON data
    json_t *root;
    json_error_t error;

    if (response_data[0] == '\0') {
        fprintf(stderr, "Empty response received from the API\n");
        return;
    }

    root = json_loads(response_data, JSON_DECODE_ANY | JSON_DISABLE_EOF_CHECK, &error);
    if (!root) {
        fprintf(stderr, "Error parsing JSON: %s\n", error.text);
        return;
    }

    json_t *current_object = json_object_get(root, "current");
    if (current_object) {
        json_t *location_object = json_object_get(root, "location");
        if (location_object) {
            json_t *city_object = json_object_get(location_object, "name");
            if (json_is_string(city_object)) {
                const char *city_value = json_string_value(city_object);

                // Declare variables for temperature, feels_like, uv_index, and weather_description
                double temperature_fahrenheit = 0.0;
                double feels_like_fahrenheit = 0.0;
                int uv_index_value = 0;

                json_t *temperature_object = json_object_get(current_object, "temperature");
                if (json_is_number(temperature_object)) {
                    double temperature_value = json_number_value(temperature_object);
                    temperature_fahrenheit = celsiusToFahrenheit(temperature_value);
                } else {
                    printf("Temperature not found or not a number in JSON\n");
                }

                json_t *feels_like_object = json_object_get(current_object, "feelslike");
                if (json_is_number(feels_like_object)) {
                    double feels_like_value = json_number_value(feels_like_object);
                    feels_like_fahrenheit = celsiusToFahrenheit(feels_like_value);
                } else {
                    printf("Feels Like not found or not a number in JSON\n");
                }

                json_t *uv_index_object = json_object_get(current_object, "uv_index");
                if (json_is_number(uv_index_object)) {
                    uv_index_value = json_integer_value(uv_index_object);
                } else {
                    printf("UV Index not found or not a number in JSON\n");
                }

                json_t *weather_description_array = json_object_get(current_object, "weather_descriptions");
                if (json_is_array(weather_description_array)) {
                    size_t index;
                    json_t *value;

                    json_array_foreach(weather_description_array, index, value) {
                        if (json_is_string(value)) {
                            const char *weather_description_value = json_string_value(value);

                            // Update GTK labels with the received data
                            update_labels(city_value, temperature_fahrenheit, feels_like_fahrenheit, uv_index_value,
                                          weather_description_value);

                            // Add the new weather data to the linked list
                            WeatherData *new_weather_data = (WeatherData *)malloc(sizeof(WeatherData));
                            strcpy(new_weather_data->city, city_value);
                            new_weather_data->temperature = temperature_fahrenheit;
                            new_weather_data->feels_like = feels_like_fahrenheit;
                            new_weather_data->uv_index = uv_index_value;
                            strcpy(new_weather_data->weather_description, weather_description_value);
                            new_weather_data->next = NULL;

                            // Add the new node to the linked list
                            if (weather_data_head == NULL) {
                                weather_data_head = new_weather_data;
                            } else {
                                WeatherData *current = weather_data_head;
                                while (current->next != NULL) {
                                    current = current->next;
                                }
                                current->next = new_weather_data;
                            }
                        } else {
                            printf("Weather Descriptions value is not a string in the array\n");
                        }
                    }
                } else {
                    printf("Weather Descriptions not found or not an array in JSON\n");
                }
            } else {
                printf("City not found or not a string in JSON\n");
            }
        } else {
            printf("Location object not found in JSON\n");
        }
    } else {
        printf("Current object not found in JSON\n");
    }

    json_decref(root);
}

int main(int argc, char *argv[]) {
    // Initialize GTK
    gtk_init(&argc, &argv);

    // Create the main window
    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Weatherly");
    gtk_container_set_border_width(GTK_CONTAINER(window), 0);
    gtk_window_set_default_size(GTK_WINDOW(window), 350, 250);

    // Create labels to display information
    weather_image = gtk_image_new();  // Create the weather image widget
    weather_description_label = gtk_label_new("");
    temperature_label = gtk_label_new("");
    feels_like_label = gtk_label_new("");
    uv_index_label = gtk_label_new("");
    location_query_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(location_query_entry), "Enter City");

    GtkWidget *button = gtk_button_new_with_label("Get Weather");
    g_signal_connect(button, "clicked", G_CALLBACK(on_button_clicked), NULL);

    print_history_button = gtk_button_new_with_label("Save History to File (Linked List)");
    g_signal_connect(print_history_button, "clicked", G_CALLBACK(on_print_history_button_clicked), NULL);

    GtkWidget *vbox = gtk_vbox_new(TRUE, 0);
    GtkWidget *logo_image = create_logo_image();  // Create the logo image
    gtk_box_pack_start(GTK_BOX(vbox), logo_image, FALSE, FALSE, 0);  // Add the logo image

    gtk_box_pack_start(GTK_BOX(vbox), location_query_entry, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), weather_image, FALSE, FALSE, 0);  // Add the weather image
    gtk_box_pack_start(GTK_BOX(vbox), temperature_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), feels_like_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), uv_index_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), print_history_button, FALSE, FALSE, 0);

    // Create a notebook to hold multiple pages
    notebook = gtk_notebook_new();

    // Create the main page with the desired background color
    GdkRGBA main_page_background_color = {1.0, 0.65, 0.36, 1.0};  // Use the RGBA values for "ffa756"
    gtk_widget_override_background_color(vbox, GTK_STATE_FLAG_NORMAL, &main_page_background_color);

    GtkWidget *main_page = gtk_vbox_new(TRUE, 0);
    gtk_box_pack_start(GTK_BOX(main_page), vbox, FALSE, FALSE, 0);

    // Append the main page to the notebook
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), main_page, NULL);

    // Set the name of the first page to "Find Weather"
    gtk_notebook_set_tab_label_text(GTK_NOTEBOOK(notebook), main_page, "Find a City");

    // ... (you can add additional pages and widgets to the notebook if needed)
    // Create the second page with the desired background color
    GdkRGBA world_page_background_color = {1.0, 0.65, 0.36, 1.0};  // Use the RGBA values for "ffa756"
    GtkWidget *world_page = create_world_page(&world_page_background_color);

    // Append the second page to the notebook
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), world_page, NULL);

    // Set the name of the second page to "Hello"
    gtk_notebook_set_tab_label_text(GTK_NOTEBOOK(notebook), world_page, "Global Weather");

    // Add the notebook to the main window
    gtk_container_add(GTK_CONTAINER(window), notebook);

    // Connect the "destroy" signal of the window to the gtk_main_quit function
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    // Show all widgets
    gtk_widget_show_all(window);

    // Start the GTK main loop
    gtk_main();

    // Free memory used by the linked list before exiting
    free_weather_data();

    return 0;
}

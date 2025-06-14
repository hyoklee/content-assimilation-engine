#include "globus_common.h"
#include "globus_ftp_client.h"
#include <stdio.h>
#include <string.h> // For strcmp

// Global condition variable to signal completion
static globus_cond_t g_cond;
static globus_mutex_t g_mutex;
static globus_bool_t g_done = GLOBUS_FALSE;

// Callback function for the transfer completion
static void
download_complete_callback(
    void * user_arg,
    globus_ftp_client_handle_t * handle,
    globus_result_t                     result)
{
    globus_mutex_lock(&g_mutex);
    {
        if (result != GLOBUS_SUCCESS)
        {
            fprintf(stderr, "Download failed:\n");
            // globus_error_print_friendly(globus_error_peek(result), stderr, NULL);
	    globus_error_print_friendly(globus_error_peek(result));
        }
        else
        {
            printf("Download successful!\n");
        }
        g_done = GLOBUS_TRUE;
        globus_cond_signal(&g_cond);
    }
    globus_mutex_unlock(&g_mutex);
}

int
main(int argc, char *argv[])
{
    globus_result_t                     result;
    globus_ftp_client_handle_t          ftp_client_handle;
    globus_ftp_client_operationattr_t   op_attr;
    char * source_url;
    char * destination_url;

    if (argc != 3)
    {
        fprintf(stderr, "Usage: %s <source_gridftp_url> <destination_local_file_url>\n", argv[0]);
        fprintf(stderr, "Example: %s gsiftp://gridftp.example.org/path/to/remote_file file:///path/to/local_file\n", argv[0]);
        return 1;
    }

    source_url = argv[1];
    destination_url = argv[2];

    // 1. Initialize Globus Common and FTP Client modules
    result = globus_module_activate(GLOBUS_COMMON_MODULE);
    if (result != GLOBUS_SUCCESS)
    {
        fprintf(stderr, "Failed to activate GLOBUS_COMMON_MODULE:\n");
        globus_error_print_friendly(globus_error_peek(result));
        return 1;
    }

    result = globus_module_activate(GLOBUS_FTP_CLIENT_MODULE);
    if (result != GLOBUS_SUCCESS)
    {
        fprintf(stderr, "Failed to activate GLOBUS_FTP_CLIENT_MODULE:\n");
        globus_error_print_friendly(globus_error_peek(result));
        globus_module_deactivate(GLOBUS_COMMON_MODULE);
        return 1;
    }

    // Initialize condition variable and mutex for synchronization
    globus_cond_init(&g_cond, NULL);
    globus_mutex_init(&g_mutex, NULL);

    // 2. Initialize FTP Client Handle
    result = globus_ftp_client_handle_init(&ftp_client_handle, NULL); // No handle attributes for now
    if (result != GLOBUS_SUCCESS)
    {
        fprintf(stderr, "Failed to initialize FTP client handle:\n");
        globus_error_print_friendly(globus_error_peek(result));
        goto cleanup_modules;
    }

    // 3. Initialize Operation Attributes (optional)
    result = globus_ftp_client_operationattr_init(&op_attr);
    if (result != GLOBUS_SUCCESS)
    {
        fprintf(stderr, "Failed to initialize operation attributes:\n");
        globus_error_print_friendly(globus_error_peek(result));
        goto cleanup_handle;
    }

    // Example: Set data channel security to 'clear' (for testing, not recommended for production)
    // globus_ftp_client_operationattr_set_dc_security(&op_attr, GLOBUS_FTP_CLIENT_CLEAR);

    // Example: Set parallelism (number of parallel streams)
    // globus_ftp_client_operationattr_set_parallelism(&op_attr, 4);

    // 4. Perform the Download (get) operation
    printf("Attempting to download from %s to %s...\n", source_url, destination_url);
    result = globus_ftp_client_get(
        &ftp_client_handle,
        source_url,
        destination_url,
        &op_attr,
        download_complete_callback,
        NULL); // No user argument for the callback

    if (result != GLOBUS_SUCCESS)
    {
        fprintf(stderr, "Failed to initiate FTP GET:\n");
        // globus_error_print_friendly(globus_error_peek(result), stderr, NULL);
	globus_error_print_friendly(globus_error_peek(result));
        goto cleanup_op_attr;
    }

    // 5. Wait for the transfer to complete (signaled by the callback)
    globus_mutex_lock(&g_mutex);
    while (!g_done)
    {
        globus_cond_wait(&g_cond, &g_mutex);
    }
    globus_mutex_unlock(&g_mutex);

cleanup_op_attr:
    // 6. Destroy operation attributes
    globus_ftp_client_operationattr_destroy(&op_attr);

cleanup_handle:
    // 7. Destroy FTP Client handle
    globus_ftp_client_handle_destroy(&ftp_client_handle);

cleanup_modules:
    // Destroy mutex and condition variable
    globus_cond_destroy(&g_cond);
    globus_mutex_destroy(&g_mutex);

    // 8. Deactivate Globus modules
    globus_module_deactivate(GLOBUS_FTP_CLIENT_MODULE);
    globus_module_deactivate(GLOBUS_COMMON_MODULE);

    return (result == GLOBUS_SUCCESS) ? 0 : 1;
}

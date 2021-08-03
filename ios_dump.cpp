/*

OS dump info Object C code;)  
  
*/

#include <stdio.h>
#include <stdlib.h>
#include <libimobiledevice/libimobiledevice.h>
#include <libimobiledevice/mobilesync.h>
#include <libimobiledevice/lockdown.h>
#include <usbmuxd.h>
#include <plist/plist.h>
#define UDID 40
void close_all(mobilesync_client_t sync_client, lockdownd_client_t ldclient, idevice_t idevice, usbmuxd_device_info_t **devices)
{
    if (sync_client != NULL)
            mobilesync_client_free(sync_client);
    if (ldclient != NULL)
            lockdownd_client_free(ldclient);
    if (idevice != NULL)
            idevice_free(idevice);
    if (devices != NULL)
            usbmuxd_device_list_free(devices);
}
int main(void)
{
    int count, i, err;
    lockdownd_service_descriptor_t port = NULL;
    usbmuxd_device_info_t *devices;
    idevice_t idevice;
    idevice_connection_t connection;
    lockdownd_client_t ldclient;
    mobilesync_client_t sync_client = NULL;
    char *host_id;
    err = usbmuxd_get_device_list(&devices);
    if(err != 1)
    {
            printf("usbmuxd_get_device_list error %d, exiting.\n", err);
            close_all(NULL, NULL, NULL, &devices);
            return 0;
    }
    for(count = 0; devices[count].handle > 0; count++);
    /* First device only */
    err = idevice_new(&idevice, devices[0].udid);
    if(err != 0)
    {
            printf("idevice_new error %d, exiting.\n", err);
            close_all(NULL, NULL, idevice, &devices);
            return 0;
    }
    err = lockdownd_client_new_with_handshake(idevice, &ldclient, "mobilesync");
    if(err != 0)
    {
            printf("lockdownd_client_new_with_handshake %d, exiting.\n", err);
            close_all(NULL, ldclient, idevice, &devices);
            return 0;
    }
    userpref_get_host_id(&host_id);
    err = lockdownd_start_service(ldclient, "com.apple.mobilesync", &port);
    if(err != 0)
    {
            printf("lockdownd_start_service %d, exiting.\n", err);
            close_all(NULL, ldclient, idevice, &devices);
            return 0;
    }
    err = mobilesync_client_new(idevice, port, &sync_client);
    if(err != 0)
    {
            printf("mobilesync_client_new %d, exiting.\n", err);
            close_all(sync_client, ldclient, idevice, &devices);
            return 0;
    }
    plist_t data            = NULL;
    plist_t msg             = NULL;
    plist_t response_t      = NULL;
    char * response;
    /* Initial handshake */
    msg = plist_new_array();
    plist_array_append_item(msg, plist_new_string("SDMessageSyncDataClassWithDevice"));
    plist_array_append_item(msg, plist_new_string("com.apple.Bookmarks"));
    plist_array_append_item(msg, plist_new_string("---"));
    plist_array_append_item(msg, plist_new_string("2013-07-29 22-34-00 00"));
/*      plist_array_append_item(msg, plist_new_string("SDSyncTypeSlow"));       */
    plist_array_append_item(msg, plist_new_uint(106));
    plist_array_append_item(msg, plist_new_string("___EmptyParameterString___"));
    err = mobilesync_send(sync_client, msg);
    plist_free(msg);
    msg = NULL;
    if(err != 0)
    {
            printf("mobilesync_send %d, exiting.\n", err);
            close_all(sync_client, ldclient, idevice, &devices);
            return 0;
    }
    plist_free(msg);
    msg = NULL;
    err = mobilesync_receive(sync_client, &msg);
    if(err != 0)
    {
            printf("mobilesync_receive %d, exiting.\n", err);
            close_all(sync_client, ldclient, idevice, &devices);
            return 0;
    }
    response_t = plist_array_get_item(msg, 0);
    plist_get_string_val(response_t, &response);
    if(strcmp(response, "SDMessageSyncDataClassWithComputer") != 0)
    {
            close_all(sync_client, ldclient, idevice, &devices);
            return 0;
    }
    msg = plist_new_array();
    plist_array_append_item(msg, plist_new_string("SDMessageGetAllRecordsFromDevice"));
    plist_array_append_item(msg, plist_new_string("com.apple.Bookmarks"));
    err = mobilesync_send(sync_client, msg);
    plist_free(msg);
    msg = NULL;
    if(err != 0)
    {
            printf("mobilesync_send %d, exiting.\n", err);
            close_all(sync_client, ldclient, idevice, &devices);
            return 0;;
    }
    char * xml = NULL;
    unsigned int xml_size;
    err = mobilesync_receive(sync_client, &msg);
    if(err != 0)
    {
            printf("mobilesync_receive %d, exiting.\n", err);
            close_all(sync_client, ldclient, idevice, &devices);
            return 0;
    }
    /* both checks are voluntary */
    if(plist_get_parent(msg) == NULL)                       /* check whether it's a root node       */
            if(plist_get_node_type(msg) == PLIST_ARRAY)     /* check whether root node is an array  */
                    plist_to_xml(msg, &xml, &xml_size);
    printf("%s\n", xml);
    close_all(sync_client, ldclient, idevice, &devices);
    return 0;
}

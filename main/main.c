#include <stdio.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_spiffs.h"
#include "esp_timer.h"
#include <sqlite3.h>
#include "freertos/freeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"


TaskHandle_t myTask1Handle = NULL;
TaskHandle_t myTask2Handle = NULL;
QueueHandle_t queue1;
static const char *tag = "example";

const char* data = "Callback function called";

static int callback(void *data, int argc, char **argv, char **azColName) {
   int i;
   printf("%s: ", (const char*)data);
   for (i = 0; i<argc; i++){
       printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
   }
   printf("\n");
   return 0;
}
int db_open(const char *filename, sqlite3 **db) {
   int rc = sqlite3_open(filename, db);
   if (rc) {
       printf("Can't open database: %s\n", sqlite3_errmsg(*db));
       return rc;
   } else {
       printf("Opened database successfully\n");
   }
   return rc;
}

char *zErrMsg = 0;
int db_exec(sqlite3 *db, const char *sql) {
   printf("%s\n", sql);
   int64_t start = esp_timer_get_time();
   int rc = sqlite3_exec(db, sql, callback, (void*)data, &zErrMsg);
   if (rc != SQLITE_OK) {
       printf("SQL error: %s\n", zErrMsg);
       sqlite3_free(zErrMsg);
   } else {
       printf("Operation done successfully\n");
   }
   printf("Time taken: %lld\n", esp_timer_get_time()-start);
   return rc;
}

void task1(void *arg)
{

char txbuff[100];

queue1= xQueueCreate(5, sizeof(txbuff));

 if( queue1 == 0 )
 {
    printf("failed to create queue1= %p \n",queue1); // Failed to create the queue.

 }



sprintf(txbuff,"INSERT INTO test1 VALUES (1, 'Hello, World from kaustubh');");
xQueueSend(queue1, (void*)txbuff , (TickType_t)0 );


sprintf(txbuff,"INSERT INTO test1 VALUES (2, 'Hello, World from kush');");
xQueueSend(queue1, (void*)txbuff , (TickType_t)0 );

sprintf(txbuff,"INSERT INTO test1 VALUES (3, 'Hello, World from shivam');");
xQueueSend(queue1, (void*)txbuff , (TickType_t)0 );


sprintf(txbuff,"SELECT * FROM test1");
xQueueSend(queue1, (void*)txbuff , (TickType_t)0 );

 while(1){

   printf("data waiting to be read : %d  available spaces: %d \n",uxQueueMessagesWaiting(queue1),uxQueueSpacesAvailable(queue1));

   vTaskDelay(pdMS_TO_TICKS(1000)); }
}

void task2(void *arg)
{
char rxbuff[100];
sqlite3 *db1;
sqlite3_initialize();
db_open("/spiffs/test2.db", &db1);
while(1)

{
//if(xQueuePeek(queue1, &(rxbuff) , (TickType_t)5 ))
if(xQueueReceive(queue1, &(rxbuff) , (TickType_t)5 ))
{

	db_exec(db1, rxbuff);

	/*FILE* fd = fopen("/spiffs/hello.txt", "w");
	int len = strlen(rxbuff);
	fwrite(rxbuff, 1, len, fd);
	fclose(fd);
	printf("got a data from queue!  ===  %s \n",rxbuff); */

}
   vTaskDelay(pdMS_TO_TICKS(1000));
}
sqlite3_close(db1);
}

void app_main()
{
	ESP_LOGI(tag, "==== STARTING SPIFFS TEST ====\n");

    esp_vfs_spiffs_conf_t conf = {
      .base_path = "/spiffs",
      .partition_label = NULL,
      .max_files = 5,
      .format_if_mount_failed = true
    };

    // Use settings defined above to initialize and mount SPIFFS filesystem.
    // Note: esp_vfs_spiffs_register is an all-in-one convenience function.
    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(tag, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(tag, "Failed to find SPIFFS partition");
        } else {
            ESP_LOGE(tag, "Failed to initialize SPIFFS (%d)", ret);
        }
        return 0;
    }

   xTaskCreate(task1, "task1", 4096, NULL, 10, &myTask1Handle);
   xTaskCreatePinnedToCore(task2, "task2", 4096, NULL, 10, &myTask2Handle,1);
}


#include <stdio.h>
#include <usb.h>

#define VENDOR_ID    0x04B4
#define PRODUCT_ID   0x7200

#define BUFSIZE      10000
#define TIMEOUT      5000


void save_buf(char *buf, int length, char *filename) {
  FILE *f;
  f = fopen(filename, "w");
  if (f == NULL) {
    fprintf(stderr, "Cannot open file \"%s\" for writing.\n", filename);
    return;
  }

  if (fwrite(buf, 1, length, f) != length) {
    fprintf(stderr, "Error writing buffer!\n");
  }

  fclose(f);
}



int main(int argc, char *argv[]) {
  int res;
  char *dumpfilename;
  char buf[BUFSIZE];

  struct usb_bus *p;
  struct usb_device *q;
  struct usb_device *current_device;
  usb_dev_handle *current_handle;

  if (argc < 2) {
    printf("Too few arguments.\n");
    exit(1);
  }

  dumpfilename = argv[1];

  usb_init();
  usb_find_busses();
  usb_find_devices();

  p = usb_busses;
  current_device = NULL;
  while (p != NULL) {
    q = p->devices;
    while (q != NULL) {
      if ((q->descriptor.idVendor == VENDOR_ID) && (q->descriptor.idProduct == PRODUCT_ID)) {
	current_device = q;
      }
      q = q->next;
    }
    p = p->next;
  }

  if (current_device == NULL) {
    printf("Could not find a CY7C68013!\n");
    exit(1);
  } else {
    printf("Found CY7C68013.\n");
  }

  current_handle = usb_open(current_device);
  res = usb_claim_interface(current_handle, 0);

  if (res < 0) {
    printf("Error claiming interface!\n");
    exit(1);
  }
  
  res = usb_get_string_simple(current_handle, 1, buf, BUFSIZE);

  if (res < 0) {
    printf("Error reading manufacturer ID!\n");
    exit(1);
  }

  printf("ID='%s'\n", buf);
  
  res = usb_control_msg(current_handle, USB_ENDPOINT_IN | USB_TYPE_VENDOR | USB_RECIP_DEVICE,
			0xFF, 0x0007, 0x03e8, buf, 2048, TIMEOUT); // 0 was 0xfff8

  if (res < 0) {
    printf("Error reading splat!\n");
    exit(1);
  }

  printf("read bytes='%d'\n", res);

  save_buf(buf, 2048, dumpfilename);

  usb_release_interface(current_handle, 0);
  usb_close(current_handle);

  

  return 0;
}

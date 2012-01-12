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



// int main()
// {
//   struct usb_bus *p;
//   struct usb_device *q;
//   struct usb_device *current_device;
//   usb_dev_handle *current_handle;
//   unsigned char firmware[37]=
//     {0x90, 0xE6, 0x0B, 0x74, 0x03, 0xF0,	//REVCTL=0x03
//      0x90, 0xE6, 0x04, 0x74, 0x80, 0xF0,	//FIFORESET=0x80
//      0x74, 0x08, 0xF0,			//FIFORESET=0x08
//      0xE4, 0xF0,				//FIFORESET=0x00
//      0x90, 0xE6, 0x01, 0x74, 0xCB, 0xF0,	//IFCONFIG=0xCB
//      0x90, 0xE6, 0x1B, 0x74, 0x0D, 0xF0,	//EP8FIFOCFG=0x0D
//      0x90, 0xE6, 0x09, 0x74, 0x10, 0xF0,	//FIFOPINPOLAR=0x10   TRUST!!!
//      0x80, 0xFE};				//while (1) {}
//   unsigned char reset[2]={1,0};
//   unsigned char buffer[10000];
//   int er[10];
//   int endpoint=8;
//   int i,tlen;

//   usb_init();
//   er[2]=usb_find_busses();
//   er[3]=usb_find_devices();

//   p=usb_busses;
//   current_device=NULL;
//   while(p!=NULL)
//     {q=p->devices;
//       while(q!=NULL)
// 	{if ((q->descriptor.idVendor==0x4b4)&&(q->descriptor.idProduct==0x7200))
// 	    current_device=q;
// 	  q=q->next;}
//       p=p->next;}
//   if (current_device==NULL)
//     {printf("\n\nCould not find a CY7C68013\n\n");exit(0);}
//   fflush(stdout);

//   current_handle=usb_open(current_device);

//   er[4]=usb_control_msg(current_handle, 0x40, 0xa0, 0xE600, 0, reset, 1, 1000);     //RESET
//   sleep(0.1);

//   for(i=0;i<37;i+=16)		//LOAD FIRMWARE
//     {tlen=60-i;
//       if(tlen>16) tlen=16;
//       er[5]=usb_control_msg(current_handle, 0x40, 0xa0, i, 0, firmware+i, tlen, 1000);}


//   er[6]=usb_control_msg(current_handle, 0x40, 0xa0, 0xE600, 0, reset+1, 1, 1000);   //UNRESET
//   sleep(0.1);

//   er[7]=usb_claim_interface(current_handle, 0);
//   er[8]=usb_set_altinterface(current_handle, 1);

//   er[9]=usb_bulk_read(current_handle, endpoint, buffer, 512, 1000);

//   for (i=0;i<512;i++) printf(" %02x", buffer[i]); printf("\n");

//   usb_release_interface(current_handle, 0);
//   usb_close(current_handle);

//   printf("\n status: ");for (i=1;i<11;i++) printf (" %d",er[i]); printf("\n\n");

//   return 0;
// }

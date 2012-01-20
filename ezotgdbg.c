/**************************************************************************
 *                                                                        *
 *        Linux firmware tool for the Cypress EZ-OTG / EZ-Host.           *
 *        The chip found, among other places, on the Xilinx ML501.        *
 *                                                                        *
 **************************************************************************
 *                (c) Copyright 2012 Stanislav Datskovskiy                *
 *                         http://www.loper-os.org                        *
 **************************************************************************
 *                                                                        *
 *  This program is free software: you can redistribute it and/or modify  *
 *  it under the terms of the GNU General Public License as published by  *
 *  the Free Software Foundation, either version 3 of the License, or     *
 *  (at your option) any later version.                                   *
 *                                                                        *
 *  This program is distributed in the hope that it will be useful,       *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *  GNU General Public License for more details.                          *
 *                                                                        *
 *  You should have received a copy of the GNU General Public License     *
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 *                                                                        *
 *************************************************************************/

#include <stdio.h>
#include <usb.h>

/* Only the CY7C67300 for now, because that is what we have. */
#define VENDOR_ID              0x04B4
#define PRODUCT_ID             0x7200

/* TODO: destupidate */
#define PAGESIZE                 4096
#define BUFSIZE                 99999
#define TIMEOUT                 50000

/* ... you will not ask about the Secret Ingredient! */
#define CY_REQUEST_CODE          0xFF
#define CY_READ_EEPROM         0x0007
#define CY_SHORT_WRITE_EEPROM  0x0008
#define CY_LONG_WRITE_EEPROM   0x0000

#define CY_SCAN_SIGNATURE      0xC3B6
#define CY_OPCODE_COPY              0
#define CY_OPCODE_JUMP              4
#define CY_OPCODE_CALL              5


/* TODO: find out if this is EXACTLY the same for ALL long writes! */

#define CY_LONG_SCAN_BOOTSTRAP_LENGTH  144
unsigned char long_scan_bootstrap[CY_LONG_SCAN_BOOTSTRAP_LENGTH] =
  {0xb6, 0xc3, 0x04, 0x00, 0x00, 0x42, 0x00, /* op=0: copy 4-2 bytes to 0x0042: 4c 03 */
   0x4c, 0x03,
   /* op=0: copy 112 bytes to 0x0310 */
   0xb6, 0xc3, 0x72, 0x00, 0x00, 0x10, 0x03,
   /* Begin 112 bytes */
   0x00, 0x90, 0xc9, 0x07, 0x10, 0x02, 0xd1, 0x07, 0x42, 0x00, 0x31, 0x00, 0x0c, 0x00, 0x9f, 0xaf,
   0x2e, 0x03, 0xc9, 0x07, 0x90, 0x02, 0xd1, 0x07, 0x42, 0x00, 0x31, 0x00, 0x0c, 0x00, 0xf1, 0x07,
   0x56, 0x04, 0x02, 0x00, 0xf1, 0x07, 0x40, 0x00, 0x04, 0x00, 0xd1, 0x97, 0x41, 0x00, 0x97, 0xcf,
   0x4d, 0xaf, 0xc9, 0x07, 0x90, 0x02, 0xca, 0x07, 0xb0, 0xc0, 0x05, 0xcf, 0x4d, 0xaf, 0xc9, 0x07,
   0x10, 0x02, 0xca, 0x07, 0x90, 0xc0, 0xd2, 0x07, 0x02, 0x00, 0xc6, 0x07, 0x40, 0x00, 0xc8, 0x07,
   0x56, 0x04, 0x02, 0x0a, 0x41, 0x0c, 0x0c, 0x00, 0xc0, 0x07, 0x01, 0x00, 0x41, 0xaf, 0x31, 0xd8,
   0x0c, 0x00, 0x06, 0xda, 0x76, 0xc1, 0x9f, 0xaf, 0x2e, 0x03, 0x4e, 0xaf, 0xc0, 0xdf, 0x97, 0xcf,
   /* end of 112 bytes */
   0xb6, 0xc3, 0x04, 0x00, 0x00, 0x52, 0x00, 0x40, 0x03, /* op=0: copy 4-2 bytes to 0x0052: 40 03 */
   0xb6, 0xc3, 0x02, 0x00, 0x05, 0x10, 0x03}; /* op=5: call address 0x0310 */


/*
send eeprom.bin:
----------------
SetupPacket:
0000: 00 ff 08 00 f0 d5 00 00 
bmRequestType: 00
  DIR: Host-To-Device
  TYPE: Standard
  RECIPIENT: Device
bRequest: ff  
  unknown!


TransferBuffer: 0x0000003e (62) length
----
b6 c3 37 00 08 41 00 (op=8: move 55 bytes to address 0x0041)
----
payload:
00 b6 c3 24 00 00 c8 3f c0 09 3a c0 c0 87 07 00 27 00
3a c0 9f af c6 e7 c2 07 02 00 c0 07 00 00 71 af cf 07
00 04 c0 df 47 af b6 c3 02 00 04 c8 3f 00 00 00 00 00
00
----
 */



/*
send eeprom_scan.bin:
----------------

SetupPacket:
0000: 00 ff 08 00 f0 d5 00 00 
bmRequestType: 00
  DIR: Host-To-Device
  TYPE: Standard
  RECIPIENT: Device
bRequest: ff  
  unknown!


TransferBuffer: 0x00000069 (105) length
----
b6 c3 62 00 08 41 00 00
----------------------------------------
payload:
b6 c3 04 00 00 00 e0 00 
00
b6 c3 0a 00 00 01 3f e7 07 b3 23 3a c0 97 cf 
b6 c3 02 00 05 01 3f
b6 c3 30 00 00 01 3f
b6 c3 20 00 00 08 3f
c0 09 3a c0 c0 87 07 00 27 00 3a
c0 c2 07 02 00 c0 07 00 00 71 af cf 07 00 04 c0 
df 47 af
b6 c3 02 00 04 08 3f 00 00
b6 c3 02 00 05 01 3f 00 00 00 00 00 00
----------------------------------------
 */



struct usb_device *current_device;
usb_dev_handle *devh;
char buf[BUFSIZE];
char *filename = NULL;
int offset = 0, bytecount = 0, pagecount = 0, lastpage = 0;


void print_usage(int argc, char *argv[]) {
  printf("\n--- EZ-OTG Debug 1.0 ---\n");
  printf("(c) Copyright 2012 Stanislav Datskovskiy.\n");
  printf("http://www.loper-os.org\n\n");
  printf("Usage: %s OPERATION [OPTIONS] -[r|w] FILE\n", argv[0]);
  printf("  Options:\n");
  printf("      -r\tRead bytes from EEPROM and save to FILE.\n");
  printf("      -w\tWrite contents of FILE to EEPROM.\n");
  printf("      -o\tOffset in EEPROM to read or write from.\n");
  printf("      -n\tNumber of bytes to read or write.\n");
  printf("      -h\tShow this help screen.\n");
  printf("\n");
}


void save_buf(char *buffer, int length, char *file_name) {
  FILE *f;
  f = fopen(file_name, "w");
  if (f == NULL) {
    fprintf(stderr, "Cannot open file \"%s\" for writing.\n", filename);
    exit(1);
  }
  if (fwrite(buffer, 1, length, f) != length) {
    fprintf(stderr, "Error writing buffer!\n");
  }
  fclose(f);
}


int load_buffer(char *buffer, char *file_name) {
  FILE *f;
  int fsize, result;
  if (file_name == NULL) {
    printf("Must specify file_name!\n");
    error(1);
  }
  f = fopen(file_name, "r");
  if (f == NULL) {
    fprintf(stderr, "Cannot open file \"%s\" for reading.\n", file_name);
    exit(1);
  } else {
    fseek(f, 0, SEEK_END);
    fsize = ftell(f);
    rewind(f);
    result = fread(buffer, 1, fsize, f);
    if (result != fsize) {
      fprintf(stderr, "Error reading file \"%s\".\n", file_name);
      exit(1);
    }
    fclose(f);
  }
}


/* TODO: specify bus and device ID if more than one EZ-OTG were to hang off the bus. */
void usb_connect() {
  int conn_res;
  struct usb_bus *p;
  struct usb_device *q;
  usb_init();
  usb_find_busses();
  usb_find_devices();
  p = usb_busses;
  current_device = NULL;
  while (p != NULL) {
    q = p->devices;
    while (q != NULL) {
      if ((q->descriptor.idVendor == VENDOR_ID) &&
          (q->descriptor.idProduct == PRODUCT_ID)) {
	current_device = q;
      }
      q = q->next;
    }
    p = p->next;
  }
  if (current_device == NULL) {
    printf("Could not find an EZ-OTG chip!\n");
    exit(1);
  }
  printf("Found EZ-OTG chip.\n");
  devh = usb_open(current_device);
  conn_res = usb_claim_interface(devh, 0);
  if (conn_res < 0) {
    printf("Error claiming interface!\n");
    exit(1);
  }
  conn_res = usb_get_string_simple(devh, 1, buf, BUFSIZE);
  if (conn_res < 0) {
    printf("Error reading manufacturer ID!\n");
    exit(1);
  }
  printf("Chip Manufacturer ID='%s'\n", buf);
}


void usb_disconnect() {
  usb_release_interface(devh, 0);
  usb_close(devh);
}


void read_page(int loc, char *buffer, int length) {
  int read_res;
  read_res = usb_control_msg(devh, USB_ENDPOINT_IN | USB_TYPE_VENDOR | USB_RECIP_DEVICE,
			     CY_REQUEST_CODE, CY_READ_EEPROM, loc, buffer, length, TIMEOUT);
  if (read_res != length) {
    printf("Error reading EEPROM: %d bytes read out of %d page total.", read_res, length);
    usb_disconnect();
    exit(1);
  }
}


void read_eeprom() {
  int i;
  usb_connect();
  printf("Reading %d bytes (<= %d pages) from offset %x (%d)...\n", bytecount, pagecount, offset, offset);
  for (i = 0; i < pagecount; i++) {
    read_page(offset + (i * PAGESIZE), buf + (i * PAGESIZE), PAGESIZE);
  }
  if (lastpage > 0) read_page(offset + (pagecount * PAGESIZE), buf + (pagecount * PAGESIZE), lastpage);
  save_buf(buf, bytecount, filename);
  printf("%d bytes read OK, saved to '%s'.\n", bytecount, filename);
  usb_disconnect();
}


void send_scan_sig() {
  /* 0xb6, 0xc3, 0x02, 0x00, 0x05, 0x10, 0x03
                 \......../  \../  \......../
		 length = 2  typ 5 call addr = 0x0310 (784)
		          \...................\..../
			                      2 bytes
  */
  

}


/* TODO: implement writing. */
int main(int argc, char *argv[]) {
  char c;
  while ((c = getopt(argc, argv, "o:n:r:w:h|help")) != -1) {
    switch(c) {
    case 'o':
      offset = atoi(optarg); /* TODO: hex offset */
      break;
    case 'n':
      bytecount = atoi(optarg);
      pagecount = bytecount / PAGESIZE;
      lastpage = bytecount % PAGESIZE;
      break;
    case 'r':
      filename = optarg;
      read_eeprom();
      exit(0);
      break;
    case 'w':
      filename = optarg;
      printf("Not implemented yet!\n");
      exit(1);
      break;
    case 'h':
    default:
      print_usage(argc, argv);
      exit(0);
      break;
    }
  }
  print_usage(argc, argv);
  exit(0);
  return 0;
}

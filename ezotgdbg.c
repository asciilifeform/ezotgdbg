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
#define VENDOR_ID                 0x04B4
#define PRODUCT_ID                0x7200

#define PAGESIZE                    4096
#define MAX_ROM_SIZE               65536
#define BUFSIZE                    99999  /* TODO: destupidate */
#define TIMEOUT                    50000  /* Please, stay awhile... stay Forever! */

/*****************************************************************************/
/* ... you should not ask about the Secret Ingredient! */
#define CY_REQUEST_CODE             0xFF
#define CY_READ_MEM               0x0007
#define CY_SHORT_WRITE_EEPROM     0x0008
#define CY_LONG_WRITE_EEPROM      0x0000
#define CY_LONG_WRITE_ENDPOINT         1
#define CY_BOOTSTRAPPER_OFFSET    0x0001
#define CY_SHORT_WRITE_OFFSET     0xD5F0

#define CY_LONG_SCAN_BOOTSTRAP_LENGTH  144
unsigned char long_scan_bootstrap[CY_LONG_SCAN_BOOTSTRAP_LENGTH] =
  {0xb6, 0xc3, 0x04, 0x00, 0x00, 0x42, 0x00, /* op=0: copy 4-2 bytes to 0x0042: 4c 03 */
   0x4c, 0x03,
   /* op=0: copy 114-2 bytes to 0x0310 */
   0xb6, 0xc3, 0x72, 0x00, 0x00, 0x10, 0x03,
   /* Begin payload: 112 bytes */
   0x00, 0x90, 0xc9, 0x07, 0x10, 0x02, 0xd1, 0x07, 0x42, 0x00, 0x31, 0x00, 0x0c, 0x00, 0x9f, 0xaf,
   0x2e, 0x03, 0xc9, 0x07, 0x90, 0x02, 0xd1, 0x07, 0x42, 0x00, 0x31, 0x00, 0x0c, 0x00, 0xf1, 0x07,
   0x56, 0x04, 0x02, 0x00, 0xf1, 0x07, 0x40, 0x00, 0x04, 0x00, 0xd1, 0x97, 0x41, 0x00, 0x97, 0xcf,
   0x4d, 0xaf, 0xc9, 0x07, 0x90, 0x02, 0xca, 0x07, 0xb0, 0xc0, 0x05, 0xcf, 0x4d, 0xaf, 0xc9, 0x07,
   0x10, 0x02, 0xca, 0x07, 0x90, 0xc0, 0xd2, 0x07, 0x02, 0x00, 0xc6, 0x07, 0x40, 0x00, 0xc8, 0x07,
   0x56, 0x04, 0x02, 0x0a, 0x41, 0x0c, 0x0c, 0x00, 0xc0, 0x07, 0x01, 0x00, 0x41, 0xaf, 0x31, 0xd8,
   0x0c, 0x00, 0x06, 0xda, 0x76, 0xc1, 0x9f, 0xaf, 0x2e, 0x03, 0x4e, 0xaf, 0xc0, 0xdf, 0x97, 0xcf,
   /* end of payload */
   0xb6, 0xc3, 0x04, 0x00, 0x00, 0x52, 0x00, /* op=0: copy 4-2 bytes to 0x0052: 40 03 */
   0x40, 0x03,
   0xb6, 0xc3, 0x02, 0x00, 0x05, 0x10, 0x03}; /* op=5: call address 0x0310 */
/*****************************************************************************/

/*
  Long Send Payload (more or less):

  00 90       	xor r0,r0
  c9 07 10 02 	mov r9,0x210
  d1 07 42 00 	mov w[r9],0x42
  31 00 0c 00 	mov w[r9+0xc],r0
  9f af 2e 03 	call 0x32e
  c9 07 90 02 	mov r9,0x290
  d1 07 42 00 	mov w[r9],0x42
  31 00 0c 00 	mov w[r9+0xc],r0
  f1 07 56 04 	mov w[r9+0x2],0x456
  02 00 
  f1 07 40 00 	mov w[r9+0x4],0x40
  04 00 
  d1 97 41 00 	xor w[r9],0x41
  97 cf       	ret
  4d af       	int 0x4d
  c9 07 90 02 	mov r9,0x290
  ca 07 b0 c0 	mov r10,0xffffc0b0
  05 cf       	jmps 0x56
  4d af       	int 0x4d
  c9 07 10 02 	mov r9,0x210
  ca 07 90 c0 	mov r10,0xffffc090
  d2 07 02 00 	mov w[r10],0x2
  c6 07 40 00 	mov r6,0x40
  c8 07 56 04 	mov r8,0x456
  02 0a       	mov r2,b[r8++]
  41 0c 0c 00 	mov r1,w[r9+0xc]
  c0 07 01 00 	mov r0,0x1
  41 af       	int 0x41
  31 d8 0c 00 	inc w[r9+0xc]
  06 da       	dec r6
  76 c1       	jnzs 0x62
  9f af 2e 03 	call 0x32e
  4e af       	int 0x4e
  c0 df       	sti
  97 cf       	ret
*/


struct usb_device *current_device;
usb_dev_handle *devh;
char buf[BUFSIZE];
char *filename = NULL;
unsigned int offset = 0, bytecount = 0, pagecount = 0, lastpage = 0;


void print_usage(int argc, char *argv[]) {
  printf("\n--- EZ-OTG Debug 1.0 ---\n");
  printf("(c) Copyright 2012 Stanislav Datskovskiy.\n");
  printf("http://www.loper-os.org\n\n");
  printf("Usage: %s OPERATION [OPTIONS] -[r|w] FILE\n", argv[0]);
  printf("  Options:\n");
  printf("      -r\tRead bytes from RAM and save to FILE.\n");
  printf("      -w\tWrite contents of FILE to EEPROM via I2C.\n");
  printf("      -m\tWrite contents of (small) FILE to EEPROM.\n");
  printf("      -o\tOffset in RAM to read from.\n");
  printf("      -n\tNumber of bytes (to read.)\n");
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
  if (fsize > MAX_ROM_SIZE) {
    fprintf(stderr, "This ROM image is too big!\n");
    exit(1);
  }
  /* Pad buffer */
  buffer[fsize + 1] = 0x00;
  buffer[fsize + 2] = 0x00;
  fsize += 2;
  return fsize;
}


/* TODO: specify bus and device ID if more than one EZ-OTG were to hang off the bus. */
void usb_connect() {
  int conn_res;
  char id_buf[1024];
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
  conn_res = usb_get_string_simple(devh, 1, id_buf, BUFSIZE);
  if (conn_res < 0) {
    printf("Error reading manufacturer ID!\n");
  } else {
    printf("Chip Manufacturer ID='%s'\n", id_buf);
  }
}


void usb_disconnect() {
  usb_release_interface(devh, 0);
  usb_close(devh);
}


void read_page(int loc, char *buffer, int length) {
  int read_res;
  read_res = usb_control_msg(devh, USB_ENDPOINT_IN | USB_TYPE_VENDOR | USB_RECIP_DEVICE,
			     CY_REQUEST_CODE, CY_READ_MEM, loc, buffer, length, TIMEOUT);
  if (read_res != length) {
    printf("Error reading EEPROM: %d bytes read out of %d page total.\n", read_res, length);
    usb_disconnect();
    exit(1);
  }
  printf("Read page: %d bytes.\n", length);
}


void read_ram() {
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


void write_page(int loc, char *buffer, int length) {
  int write_res;
  printf("Writing page... (DO NOT INTERRUPT!)\n");
  write_res = usb_bulk_write(devh, CY_LONG_WRITE_ENDPOINT, buffer, length, TIMEOUT);
  if (write_res != length) {
    printf("Error sending page for I2C write: %d bytes sent out of %d page total.\n", write_res, length);
    usb_disconnect();
    exit(1);
  }
  printf("Wrote I2C page: %d bytes.\n", length);
}


void write_eeprom_long() {
  int i, size, write_res;
  size = load_buffer(buf, filename);
  usb_connect();
  /* Send bootstrapper. */
  write_res = usb_control_msg(devh, USB_ENDPOINT_OUT | USB_TYPE_VENDOR | USB_RECIP_DEVICE,
			      CY_REQUEST_CODE, CY_LONG_WRITE_EEPROM, CY_BOOTSTRAPPER_OFFSET,
			      long_scan_bootstrap, CY_LONG_SCAN_BOOTSTRAP_LENGTH, TIMEOUT);
  if (write_res != CY_LONG_SCAN_BOOTSTRAP_LENGTH) {
    printf("Error writing I2C write bootstrapper block: %d bytes sent out of %d bytes attempted.\n",
	   write_res, CY_LONG_SCAN_BOOTSTRAP_LENGTH);
    usb_disconnect();
    exit(1);
  }
  printf("Wrote I2C bootstrappper OK.\n");
  /* Calculate pages. */
  pagecount = size / PAGESIZE;
  lastpage = size % PAGESIZE;
  /* Send payload. */
  for (i = 0; i < pagecount; i++) {
    write_page(offset + (i * PAGESIZE), buf + (i * PAGESIZE), PAGESIZE);
  }
  if (lastpage > 0) write_page(offset + (pagecount * PAGESIZE), buf + (pagecount * PAGESIZE), lastpage);
  printf("%d bytes (%d pages) wrote to I2C from '%s'.\n", size, pagecount, filename);
  usb_disconnect();
}


void write_eeprom_short() {
  int size, write_res;
  size = load_buffer(buf + 8, filename); /* Reserve space for SCAN header. */
  /* pad payload by another 2 bytes (4 total, based on observations) */
  buf[size + 1] = 0x00;
  buf[size + 2] = 0x00;
  size += 2;
  /* SCAN header signature */
  buf[0] = 0xB6;
  buf[1] = 0xC3;
  /* Payload size */
  buf[2] = 0xFF & (size + 1);
  buf[3] = (0xFF00 & (size + 1)) >> 8;
  /* operation: move with interrupt */
  buf[4] = 0x08;
  /* int=0x41 */
  buf[5] = 0x41;
  /* Destination */
  buf[6] = 0x00;
  buf[7] = 0x00;
  /* Adjust byte count to fit SCAN header. */
  size += 8;
  /* Send payload. */
  usb_connect();
  write_res = usb_control_msg(devh, USB_ENDPOINT_OUT | USB_TYPE_VENDOR | USB_RECIP_DEVICE,
			      CY_REQUEST_CODE, CY_SHORT_WRITE_EEPROM, CY_SHORT_WRITE_OFFSET,
			      buf, size, TIMEOUT);

  if (write_res != size) {
    printf("Error writing EEPROM using SHORT write: %d bytes sent out of %d bytes attempted.\n",
	   write_res, size);
    usb_disconnect();
    exit(1);
  }
  printf("%d bytes wrote to I2C using SHORT write from '%s'.\n", size, filename);
  usb_disconnect();
}


/* TODO: implement writing. */
int main(int argc, char *argv[]) {
  int ok;
  char c;
  while ((c = getopt(argc, argv, "o:n:r:w:m:h|help")) != -1) {
    switch(c) {
    case 'o':
      /* offset = atoi(optarg); */
      ok = sscanf(optarg, "%x", &offset);
      if (ok != 1) {
	fprintf(stderr, "Offset '%s' is not a valid hex value!\n", optarg);
	exit(1);
      }
      break;
    case 'n':
      bytecount = atoi(optarg);
      if (bytecount > MAX_ROM_SIZE) {
	fprintf(stderr, "%d bytes is bigger than any possible attached ROM!\n", bytecount);
	exit(1);
      }
      break;
    case 'r':
      filename = optarg;
      pagecount = bytecount / PAGESIZE;
      lastpage = bytecount % PAGESIZE;
      read_ram();
      exit(0);
      break;
    case 'w':
      filename = optarg;
      write_eeprom_long();
      exit(0);
      break;
    case 'm':
      filename = optarg;
      write_eeprom_short();
      exit(0);
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

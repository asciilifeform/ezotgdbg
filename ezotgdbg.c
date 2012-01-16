/**************************************************************************
 *                                                                        *
 *        Linux debug tool for the Cypress EZ-OTG / EZ-Host.              *
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
#define VENDOR_ID    0x04B4
#define PRODUCT_ID   0x7200

/* TODO: destupidate */
#define BUFSIZE      32767
#define TIMEOUT      50000

/* ... you will not ask about the Secret Ingredient! */
#define CY_REQUEST_CODE   0xFF
#define CY_READ_EEPROM    0x0007
#define CY_WRITE_EEPROM   0x0000


int res;
struct usb_device *current_device;
usb_dev_handle *devh;
char buf[BUFSIZE];
char *filename = NULL;
int offset = 0, bytecount = 0;


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


void save_buf(char *buf, int length, char *filename) {
  FILE *f;
  f = fopen(filename, "w");
  if (f == NULL) {
    fprintf(stderr, "Cannot open file \"%s\" for writing.\n", filename);
    exit(1);
  }
  if (fwrite(buf, 1, length, f) != length) {
    fprintf(stderr, "Error writing buffer!\n");
  }
  fclose(f);
}


int load_buf(char *buf, char *filename) {
  FILE *f;
  int fsize, result;
  if (filename == NULL) {
    printf("Must specify filename!\n");
    error(1);
  }
  f = fopen(filename, "r");
  if (f == NULL) {
    fprintf(stderr, "Cannot open file \"%s\" for reading.\n", filename);
    exit(1);
  } else {
    fseek(f, 0, SEEK_END);
    fsize = ftell(f);
    rewind(f);
    result = fread(buf, 1, fsize, f);
    if (result != fsize) {
      fprintf(stderr, "Error reading file \"%s\".\n", filename);
      exit(1);
    }
    fclose(f);
  }
}


/* TODO: specify bus and device ID if more than one EZ-OTG were to hang off the bus. */
void usb_connect() {
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
  res = usb_claim_interface(devh, 0);
  if (res < 0) {
    printf("Error claiming interface!\n");
    exit(1);
  }
  res = usb_get_string_simple(devh, 1, buf, BUFSIZE);
  if (res < 0) {
    printf("Error reading manufacturer ID!\n");
    exit(1);
  }
  printf("Chip Manufacturer ID='%s'\n", buf);
}


void usb_disconnect() {
  usb_release_interface(devh, 0);
  usb_close(devh);
}


void read_eeprom() {
  usb_connect();
  printf("Reading %d bytes from offset %x (%d)...\n", bytecount, offset, offset);
  res = usb_control_msg(devh, USB_ENDPOINT_IN | USB_TYPE_VENDOR | USB_RECIP_DEVICE,
 			CY_REQUEST_CODE, CY_READ_EEPROM, offset, buf, bytecount, TIMEOUT);
  if (res != bytecount) {
    printf("Error reading EEPROM: %d bytes read out of %d total.", res, bytecount);
  } else {
    save_buf(buf, bytecount, filename);
    printf("%d bytes read OK, saved to '%s'.\n", bytecount, filename);
  }
  usb_disconnect();
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
	break;
      case 'r': /* TODO: read >4KB at a time... */
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include <libftdi1/ftdi.h>
#include <libusb-1.0/libusb.h>

#define BIT(_x) (1 << _x)

// FTDI Chip Macros
#define CSN0 BIT(0)
#define CSN1 BIT(1)
#define SCLK BIT(2)
#define MISO BIT(3)
#define MOSI BIT(4)
#define PROG BIT(5)

// Flash Chip Macros
#define FLASH_DEV2B_ID 0x231F
#define FLASH_DEV8B_ID 0x251F

// Output Value Macros
#define FPGA_SEL CSN0
#define FLASH_SEL CSN1
#define IDLE_SEL (CSN1 | CSN0)

// Direction Macros
#define CONST_DIR 0x17 // CSN0-1, SCLK, MOSI outputs, MISO input
#define DEASSERT_PROG_PIN(_x) ftdi_set_bitmode(_x, CONST_DIR, BITMODE_BITBANG)
#define ASSERT_PROG_PIN(_x) ftdi_set_bitmode(_x, BIT(5) | CONST_DIR, BITMODE_BITBANG)

// Structs
typedef struct opts
{
  char * bitstream_name;
  char * eeprom_image;
  unsigned long image_start;
  int flash_image;
}OPTS;

// Globals
unsigned char serial_num[6] = {'\0'};
unsigned char file_buf[264] = {'\0'};
unsigned char bitbang_buf[5000] = {'\0'};


// Fcn Prototypes
int open_merc(struct ftdi_context * ftdic, unsigned char * serial_buf, unsigned int seral_len);
void close_merc(struct ftdi_context * ftdic);
int parse_args(int argc, char * argv[], OPTS * clopts);
void print_usage();

// SPI fcns
int SPI_sel(struct ftdi_context * ftdic, unsigned char sel);
int SPI_out(struct ftdi_context * ftdic, unsigned char * bytes, unsigned int len, unsigned char sel);
int SPI_in(struct ftdi_context * ftdic, unsigned char * bytes, unsigned int len, unsigned char sel);
int SPI_bulk(unsigned char * bytes_to_write, unsigned char * bitbang_stream, unsigned int len, unsigned char sel);

// Programmer fcns
int do_erase_cmd(struct ftdi_context * ftdic, unsigned char * cmd_string, int timeout);
unsigned int flash_ID(struct ftdi_context * ftdic);
int flash_erase(struct ftdi_context * ftdic, unsigned int len, unsigned long start_addr, unsigned int flash_id);
int flash_write(struct ftdi_context * ftdic, unsigned char * bytes, unsigned int page_addr);
int flash_poll(struct ftdi_context * ftdic, int timeout);
unsigned int flash_size(unsigned int flash_id);

int main(int argc, char * argv[])
{
  OPTS clopts;
  FILE * bitstream, * epimage;
  int rc;
  struct ftdi_context ftdic;
  unsigned int retrieved_id;
  unsigned int page_addr;
  size_t chars_read;

  (void) rc;

  if(parse_args(argc, argv, &clopts))
  {
    print_usage();
    return -1;
  }

  if(open_merc(&ftdic, serial_num, 5))
  {
    printf("Error connecting to Mercury: %s. Is device connected?\n", ftdi_get_error_string(&ftdic));
    return -1;
  }
  else
  {
    printf("Device was found okay! Serial number is %s.\n", serial_num);
  }


  bitstream = fopen(clopts.bitstream_name, "rb");
  if(bitstream == NULL)
  {
     printf("Error opening %s: %s. Aborting.\n", clopts.bitstream_name, strerror(errno));
     return -1;
  }

  if(clopts.flash_image)
  {
    epimage = fopen(clopts.eeprom_image, "rb");
    if(epimage == NULL)
    {
     printf("Error opening %s: %s. Aborting.\n", clopts.eeprom_image, strerror(errno));
     return -1;
    }
  }

  if(DEASSERT_PROG_PIN(&ftdic)) // Just in case...
  {
    printf("Error deasserting prog pin.\n");
    return -1;
  }

  retrieved_id = flash_ID(&ftdic) & 0xFFFF;

  if(!(retrieved_id == FLASH_DEV2B_ID || retrieved_id == FLASH_DEV8B_ID))
  {
    printf("Error reading Flash ID: %s. Aborting.\n", ftdi_get_error_string(&ftdic));
    return -1;
  }

  if(flash_erase(&ftdic, 0, 0, retrieved_id))
  {
    printf("Error erasing Flash: %s. Aborting.\n", ftdi_get_error_string(&ftdic));
    return -1;
  }
  else
  {
    printf("Flash erase was okay.\n");
    printf("Programming bitstream; each '*' is 4224 bytes:\n");
  }

// TODO: Strictly speaking, it's not actually possible to get the size of a binary file in C.
// Macroize away based on platform. Assume it's a valid size for now.

  page_addr = 0;
  while((chars_read = fread(file_buf, 1, 264, bitstream)) == 264)
  {
    flash_write(&ftdic, file_buf, page_addr);
    if((++page_addr % 16) == 0)
    {
      putc('*', stdout);
      fflush(stdout);
    }
  }

  putc('\n', stdout);

  if(feof(bitstream))
  {
    memset(&file_buf[chars_read], 0, 264 - chars_read);
    flash_write(&ftdic, file_buf, page_addr);
  }
  else
  {
    printf("Error reading bitstream: %s. Aborting.\n", strerror(errno));
    return -1;
  }

  printf("Mercury board programmed successfully.\n");

  if(fclose(bitstream))
  {
    printf("Warning: bitstream file close failed. Check your system.\n"
           "Exiting with success.\n");
  }

  DEASSERT_PROG_PIN(&ftdic);
  close_merc(&ftdic);


  return 0;
}

int parse_args(int argc, char * argv[], OPTS * clopts)
{
  if(argc < 2)
  {
    return -1;
  }

  if(!strcmp("-h", argv[1]))
  {
    return -1;
  }

  clopts->bitstream_name = argv[1];
  clopts->eeprom_image = NULL;
  clopts->image_start = 0;
  clopts->flash_image = 0;

  return 0;
}

void print_usage()
{
  printf("Usage: mercpcl [-h] [bitstream_file]\n");
}

int open_merc(struct ftdi_context * ftdic, unsigned char * serial_buf, unsigned int serial_len)
{
  struct libusb_device_descriptor desc;

  if(ftdi_init(ftdic) || \
       ftdi_usb_open_desc(ftdic, 0x0403, 0x6001, "Mercury FPGA", NULL) || \
       ftdi_set_baudrate(ftdic, 3000000))
  {
    return -1;
  }

  libusb_get_device_descriptor(libusb_get_device(ftdic->usb_dev), &desc);
  libusb_get_string_descriptor_ascii(ftdic->usb_dev, desc.iSerialNumber, serial_buf, serial_len);

  if(DEASSERT_PROG_PIN(ftdic))
  {
    return -1;
  }

  if(SPI_sel(ftdic, IDLE_SEL) < 0)
  {
    return -1;
  }


  return 0;
}

void close_merc(struct ftdi_context * ftdic)
{
  ftdi_deinit(ftdic);
}

int SPI_sel(struct ftdi_context * ftdic, unsigned char sel)
{
  return ftdi_write_data(ftdic, &sel, 1);
}


// If deslect should happen after calling this function, it needs to be done manually!
// Flash expects CS to stay asserted between data out and data in (PC's point-of-view).
int SPI_out(struct ftdi_context * ftdic, unsigned char * bytes, unsigned int len, unsigned char sel)
{
  int byte_count, bitmask, i;
  unsigned char byte_stream[17];

  (void) i;

  for(byte_count = 0; (unsigned int) byte_count < len; byte_count++)
  {
    int curr_byte = bytes[byte_count];
    for(bitmask = 0x80, i = 0; bitmask; bitmask >>= 1, i = i + 2)
    {
      int curr_bit = (curr_byte & bitmask) ? MOSI : 0;

      byte_stream[i]  = curr_bit | sel;
      byte_stream[i + 1]  = curr_bit | SCLK | sel;
    }

    assert(i == 16);
    byte_stream[i] = '\0';

#ifdef M_DEBUG
    for(i = 0; i < 17; i++)
    {
      printf("byte_stream[%d]: %02X\n", i, byte_stream[i]);
    }
#endif

    ftdi_write_data(ftdic, byte_stream, 17);
  }

  return 0;
}


int SPI_in(struct ftdi_context * ftdic, unsigned char * bytes, unsigned int len, unsigned char sel)
{
  int byte_count, bit_no;
  unsigned char pin_vals;

  for(byte_count = 0; (unsigned int) byte_count < len; byte_count++)
  {
    char byte_read = 0;

    for(bit_no = 7; bit_no >= 0; bit_no--)
    {
      unsigned char pin_vals;
      unsigned char clock_pulse = (SCLK | sel);

      ftdi_read_pins(ftdic, &pin_vals);
      byte_read |= ((pin_vals & MISO) ? 1 : 0) << bit_no;

      ftdi_write_data(ftdic, &clock_pulse, 1);
      clock_pulse = sel;
      ftdi_write_data(ftdic, &clock_pulse, 1);
    }

#ifdef M_DEBUG
    printf("Byte read: %hhX\n", byte_read);
#endif

    bytes[byte_count] = byte_read;
  }

  ftdi_read_pins(ftdic, &pin_vals);
  SPI_sel(ftdic, IDLE_SEL);
  return 0;

}



unsigned int flash_ID(struct ftdi_context * ftdic)
{
  unsigned char id_cmd = 0x9F;
  unsigned int id; // Assuming 32-bit int here. Should be fine but if not will change.

  if(SPI_out(ftdic, &id_cmd, 1, FLASH_SEL))
  {
    id = -1;
    goto restore;
  }

  /* Conversion from int ptr to char ptr is safe.  */
  if(SPI_in(ftdic, (unsigned char *)  &id, 4, FLASH_SEL))
  {
    id = -1;
  }

#ifdef M_DEBUG
  printf("Flash ID %08X\n", id);
#endif

restore:
  SPI_sel(ftdic, IDLE_SEL);
  return id;
}

// TODO: Meaningfully support start_addr (erase on block/page boundaries.
// Use the fewest amount of commands.
int flash_erase(struct ftdi_context * ftdic, unsigned int len, unsigned long start_addr, unsigned int flash_id)
{
  int rc = 0;
  int num_sectors;
  int i;
  unsigned char erase_sector_0a[4] = {0x7C, 0x00, 0x00, 0x00};
  unsigned char erase_sector_0b[4] = {0x7C, 0x00, 0x10, 0x00};
  unsigned char erase_sector_other[4] = {0x7C, 0x00, 0x00, 0x00};

  (void) len;
  (void) start_addr;

  ASSERT_PROG_PIN(ftdic); // Ensure FPGA ignores bus.
//  int num_sectors = ;

  if(do_erase_cmd(ftdic, erase_sector_0a, 300000) || \
       do_erase_cmd(ftdic, erase_sector_0b, 300000))
  {
    rc = 1;
    goto restore_state;
  }

  num_sectors = (flash_id == FLASH_DEV8B_ID) ? 16 : 8;

  for(i = 1; i < num_sectors; i++)
  {
    erase_sector_other[1] = i;
    if(flash_id == FLASH_DEV8B_ID)
    {
      erase_sector_other[1] = (i << 1); // PA7 => Bit 16. Sectors start at PA8.
    }
    if(do_erase_cmd(ftdic, erase_sector_other, 300000))
    {
      rc = 1;
      break;
    }
  }

restore_state:
  DEASSERT_PROG_PIN(ftdic);
  return rc;
}

int do_erase_cmd(struct ftdi_context * ftdic, unsigned char * cmd_string, int timeout)
{
  // Assume this succeeds- flash_poll will detect errors.
  SPI_out(ftdic, cmd_string, 4, FLASH_SEL);
  SPI_sel(ftdic, IDLE_SEL); // Command doesn't start until CS=>high.
  return flash_poll(ftdic, timeout);
}



// Expects a page of bytes (264)
int flash_write(struct ftdi_context * ftdic, unsigned char * bytes, unsigned int page_addr)
{
  unsigned char write_flash_buffer[4] = {0x84, 0x00, 0x00, 0x00}; // Always start at buffer addr 0.
  unsigned char buf_to_flash[4] = {0x88, 0x00, 0x00, 0x00};
  unsigned char idle[1] = {0x00}; /* OR'ed with sel in SPI_bulk to create SPI
  idle selection signal. */
  unsigned char paddr_hi, paddr_lo;
  int rc = 0;
  int bitbang_bufsiz = 0;

  paddr_hi = (((page_addr & 0x3FF) >> 7) & 0x07);
  paddr_lo = (((page_addr & 0x3FF) << 1) & 0xFE);
  buf_to_flash[1] = paddr_hi;
  buf_to_flash[2] = paddr_lo;


  ASSERT_PROG_PIN(ftdic);

#ifdef M_DEBUG
  printf("Flash write to page %d\n", page_addr);
#endif

// TODO: Remove global state
  bitbang_bufsiz = SPI_bulk(write_flash_buffer, bitbang_buf, 4, FLASH_SEL);
  bitbang_bufsiz += SPI_bulk(bytes, bitbang_buf + bitbang_bufsiz, 264, FLASH_SEL);
  bitbang_bufsiz += SPI_bulk(idle, bitbang_buf + bitbang_bufsiz, 1, IDLE_SEL);
  bitbang_bufsiz += SPI_bulk(buf_to_flash, bitbang_buf + bitbang_bufsiz, 4, FLASH_SEL);
  bitbang_bufsiz += SPI_bulk(idle, bitbang_buf + bitbang_bufsiz, 1, IDLE_SEL);

  ftdi_write_data(ftdic, bitbang_buf, 17 * (4 + 264 + 1 + 4 + 1));

  if(flash_poll(ftdic, 300000))
  {
    rc = 1;
  }

  DEASSERT_PROG_PIN(ftdic);
  return rc;
}

// Do NOT use return value for error checking- it is simply a convenience
// so that the next free index need not be calculated.
int SPI_bulk(unsigned char * bytes_to_write, unsigned char * bitbang_stream, unsigned int len, unsigned char sel)
{
  int byte_count, bitbang_offset;

  for(byte_count = 0, bitbang_offset = 0; (unsigned int) byte_count < len; byte_count++)
  {
    int i, bitmask;
    int curr_byte = bytes_to_write[byte_count];

    for(bitmask = 0x80, i = bitbang_offset; bitmask; bitmask >>= 1, i = i + 2)
    {
      int curr_bit = (curr_byte & bitmask) ? MOSI : 0;

      bitbang_stream[i]  = curr_bit | sel;
      bitbang_stream[i + 1]  = curr_bit | SCLK | sel;
    }

    assert(i == bitbang_offset + 16);
    bitbang_stream[i] = '\0'; // TODO: Is this necessary to set all bits to zero?
    bitbang_offset = i + 1;
  }

  return bitbang_offset;
}


int flash_poll(struct ftdi_context * ftdic, int timeout)
{
  unsigned char status_read = 0xD7;
  unsigned char status_code;

  int done = 0;
  int attempts = 0;

  // Assume R/Ws will succeed for simplicity.
  while(!done && (attempts < timeout))
  {
    SPI_out(ftdic, &status_read, 1, FLASH_SEL);

    // TODO: Only grab first bit?
    SPI_in(ftdic, &status_code, 1, FLASH_SEL);
    SPI_sel(ftdic, IDLE_SEL);
    if((status_code & 0x80))
    {
      done = 1;
    }
    else
    {
      attempts++;
    }
  }

#ifdef M_DEBUG
  printf("Polling took %d attempts.\n", attempts);
#endif

  return (attempts >= timeout);
}


unsigned int flash_size(unsigned int flash_id)
{
  unsigned int size; // Again, assumes 32-bit int. Will change if causes problems.

  switch(flash_id)
  {
    case FLASH_DEV8B_ID:
      size = BIT(20);
      break;
    case FLASH_DEV2B_ID:
      size = BIT(18);
      break;
    default:
      size = 0;
  }

  return size;
}

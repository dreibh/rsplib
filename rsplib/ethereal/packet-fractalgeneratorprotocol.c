#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <epan/packet.h>


#define FRACTALGENERATORPROTOCOL_PAYLOAD_PROTOCOL_ID 0x29097601


/* Initialize the protocol and registered fields */
static int proto_fractalgeneratorprotocol = -1;
static int hf_message_type                = -1;
static int hf_message_flags               = -1;
static int hf_message_length              = -1;
static int hf_data_start_x                = -1;
static int hf_data_start_y                = -1;
static int hf_data_points                 = -1;
static int hf_parameter_width             = -1;
static int hf_parameter_height            = -1;
static int hf_parameter_maxiterations    = -1;
static int hf_parameter_algorithmid       = -1;
static int hf_parameter_c1real            = -1;
static int hf_parameter_c1imag            = -1;
static int hf_parameter_c2real            = -1;
static int hf_parameter_c2imag            = -1;
static int hf_parameter_n                 = -1;

/* Initialize the subtree pointers */
static gint ett_fractalgeneratorprotocol = -1;

static void
dissect_fractalgeneratorprotocol_message(tvbuff_t *, packet_info *, proto_tree *);

#define NETWORK_BYTE_ORDER FALSE


/* Dissectors for messages. This is specific to FractalGeneratorProtocol */
#define MESSAGE_TYPE_LENGTH    1
#define MESSAGE_FLAGS_LENGTH   1
#define MESSAGE_LENGTH_LENGTH  2

#define MESSAGE_TYPE_OFFSET    0
#define MESSAGE_FLAGS_OFFSET   (MESSAGE_TYPE_OFFSET    + MESSAGE_TYPE_LENGTH)
#define MESSAGE_LENGTH_OFFSET  (MESSAGE_FLAGS_OFFSET   + MESSAGE_FLAGS_OFFSET)
#define MESSAGE_VALUE_OFFSET   (MESSAGE_LENGTH_OFFSET  + MESSAGE_LENGTH_LENGTH)


#define DATA_STARTX_LENGTH 4
#define DATA_STARTY_LENGTH 4
#define DATA_POINTS_LENGTH 4

#define DATA_STARTX_OFFSET MESSAGE_VALUE_OFFSET
#define DATA_STARTY_OFFSET (DATA_STARTX_OFFSET + DATA_STARTX_LENGTH)
#define DATA_POINTS_OFFSET (DATA_STARTY_OFFSET + DATA_STARTY_LENGTH)
#define DATA_BUFFER_OFFSET (DATA_POINTS_OFFSET + DATA_POINTS_LENGTH)


#define PARAMETER_WIDTH_LENGTH         4
#define PARAMETER_HEIGHT_LENGTH        4
#define PARAMETER_MAXITERATIONS_LENGTH 4
#define PARAMETER_ALGORITHMID_LENGTH   4
#define PARAMETER_C1REAL_LENGTH 8
#define PARAMETER_C1IMAG_LENGTH 8
#define PARAMETER_C2REAL_LENGTH 8
#define PARAMETER_C2IMAG_LENGTH 8
#define PARAMETER_N_LENGTH 8

#define PARAMETER_WIDTH_OFFSET  MESSAGE_VALUE_OFFSET
#define PARAMETER_HEIGHT_OFFSET (PARAMETER_WIDTH_OFFSET + PARAMETER_WIDTH_LENGTH)
#define PARAMETER_MAXITERATIONS_OFFSET (PARAMETER_HEIGHT_OFFSET + PARAMETER_HEIGHT_LENGTH)
#define PARAMETER_ALGORITHMID_OFFSET (PARAMETER_MAXITERATIONS_OFFSET + PARAMETER_MAXITERATIONS_LENGTH)
#define PARAMETER_C1REAL_OFFSET (PARAMETER_ALGORITHMID_OFFSET + PARAMETER_ALGORITHMID_LENGTH)
#define PARAMETER_C1IMAG_OFFSET (PARAMETER_C1REAL_OFFSET + PARAMETER_C1REAL_LENGTH)
#define PARAMETER_C2REAL_OFFSET (PARAMETER_C1IMAG_OFFSET + PARAMETER_C1IMAG_LENGTH)
#define PARAMETER_C2IMAG_OFFSET (PARAMETER_C2REAL_OFFSET + PARAMETER_C2REAL_LENGTH)
#define PARAMETER_N_OFFSET (PARAMETER_C2IMAG_OFFSET + PARAMETER_C2IMAG_LENGTH)

#define FRACTALGENERATOR_PARAMETER_MESSAGE_TYPE  0x01
#define FRACTALGENERATOR_DATA_MESSAGE_TYPE       0x02



static const value_string message_type_values[] = {
  { FRACTALGENERATOR_PARAMETER_MESSAGE_TYPE,        "FractalGenerator Parameter" },
  { FRACTALGENERATOR_DATA_MESSAGE_TYPE,             "FractalGenerator Data" },
  { 0, NULL }
};


static void
dissect_fractalgeneratorprotocol_parameter_message(tvbuff_t *message_tvb, proto_tree *message_tree)
{
  proto_tree_add_item(message_tree, hf_parameter_width,  message_tvb, PARAMETER_WIDTH_OFFSET,  PARAMETER_WIDTH_LENGTH,  NETWORK_BYTE_ORDER);
  proto_tree_add_item(message_tree, hf_parameter_height, message_tvb, PARAMETER_HEIGHT_OFFSET, PARAMETER_HEIGHT_LENGTH, NETWORK_BYTE_ORDER);
  proto_tree_add_item(message_tree, hf_parameter_maxiterations, message_tvb, PARAMETER_MAXITERATIONS_OFFSET, PARAMETER_MAXITERATIONS_LENGTH, NETWORK_BYTE_ORDER);
  proto_tree_add_item(message_tree, hf_parameter_algorithmid, message_tvb, PARAMETER_ALGORITHMID_OFFSET, PARAMETER_ALGORITHMID_LENGTH, NETWORK_BYTE_ORDER);
  proto_tree_add_item(message_tree, hf_parameter_c1real, message_tvb, PARAMETER_C1REAL_OFFSET, PARAMETER_C1REAL_LENGTH, NETWORK_BYTE_ORDER);
  proto_tree_add_item(message_tree, hf_parameter_c1imag, message_tvb, PARAMETER_C1IMAG_OFFSET, PARAMETER_C1IMAG_LENGTH, NETWORK_BYTE_ORDER);
  proto_tree_add_item(message_tree, hf_parameter_c2real, message_tvb, PARAMETER_C2REAL_OFFSET, PARAMETER_C2REAL_LENGTH, NETWORK_BYTE_ORDER);
  proto_tree_add_item(message_tree, hf_parameter_c2imag, message_tvb, PARAMETER_C2IMAG_OFFSET, PARAMETER_C2IMAG_LENGTH, NETWORK_BYTE_ORDER);
  proto_tree_add_item(message_tree, hf_parameter_n, message_tvb, PARAMETER_N_OFFSET, PARAMETER_N_LENGTH, NETWORK_BYTE_ORDER);
}


static void
dissect_fractalgeneratorprotocol_data_message(tvbuff_t *message_tvb, proto_tree *message_tree)
{
  proto_tree_add_item(message_tree, hf_data_start_x, message_tvb, DATA_STARTX_OFFSET, DATA_STARTX_LENGTH, NETWORK_BYTE_ORDER);
  proto_tree_add_item(message_tree, hf_data_start_y, message_tvb, DATA_STARTY_OFFSET, DATA_STARTY_LENGTH, NETWORK_BYTE_ORDER);
  proto_tree_add_item(message_tree, hf_data_points,  message_tvb, DATA_POINTS_OFFSET, DATA_POINTS_LENGTH, NETWORK_BYTE_ORDER);
}


static void
dissect_fractalgeneratorprotocol_message(tvbuff_t *message_tvb, packet_info *pinfo, proto_tree *fractalgeneratorprotocol_tree)
{
  guint8 type;

  type = tvb_get_guint8(message_tvb, MESSAGE_TYPE_OFFSET);
  if (pinfo && (check_col(pinfo->cinfo, COL_INFO))) {
    col_add_fstr(pinfo->cinfo, COL_INFO, "%s ", val_to_str(type, message_type_values, "Unknown FractalGeneratorProtocol type"));
  }
  if (fractalgeneratorprotocol_tree) {
    proto_tree_add_item(fractalgeneratorprotocol_tree, hf_message_type,   message_tvb, MESSAGE_TYPE_OFFSET,   MESSAGE_TYPE_LENGTH,   NETWORK_BYTE_ORDER);
    proto_tree_add_item(fractalgeneratorprotocol_tree, hf_message_flags,  message_tvb, MESSAGE_FLAGS_OFFSET,  MESSAGE_FLAGS_LENGTH,  NETWORK_BYTE_ORDER);
    proto_tree_add_item(fractalgeneratorprotocol_tree, hf_message_length, message_tvb, MESSAGE_LENGTH_OFFSET, MESSAGE_LENGTH_LENGTH, NETWORK_BYTE_ORDER);
    switch (type) {
      case FRACTALGENERATOR_PARAMETER_MESSAGE_TYPE:
        dissect_fractalgeneratorprotocol_parameter_message(message_tvb, fractalgeneratorprotocol_tree);
        break;
      case FRACTALGENERATOR_DATA_MESSAGE_TYPE:
        dissect_fractalgeneratorprotocol_data_message(message_tvb, fractalgeneratorprotocol_tree);
        break;
    }
  }
}

static void
dissect_fractalgeneratorprotocol(tvbuff_t *message_tvb, packet_info *pinfo, proto_tree *tree)
{
  proto_item *fractalgeneratorprotocol_item;
  proto_tree *fractalgeneratorprotocol_tree;

  /* pinfo is NULL only if dissect_fractalgeneratorprotocol_message is called from dissect_error cause */
  if (pinfo && (check_col(pinfo->cinfo, COL_PROTOCOL)))
    col_set_str(pinfo->cinfo, COL_PROTOCOL, "FractalGeneratorProtocol");

  /* In the interest of speed, if "tree" is NULL, don't do any work not
     necessary to generate protocol tree items. */
  if (tree) {
    /* create the fractalgeneratorprotocol protocol tree */
    fractalgeneratorprotocol_item = proto_tree_add_item(tree, proto_fractalgeneratorprotocol, message_tvb, 0, -1, FALSE);
    fractalgeneratorprotocol_tree = proto_item_add_subtree(fractalgeneratorprotocol_item, ett_fractalgeneratorprotocol);
  } else {
    fractalgeneratorprotocol_tree = NULL;
  };
  /* dissect the message */
  dissect_fractalgeneratorprotocol_message(message_tvb, pinfo, fractalgeneratorprotocol_tree);
}

/* Register the protocol with Ethereal */
void
proto_register_fractalgeneratorprotocol(void)
{

  /* Setup list of header fields */
  static hf_register_info hf[] = {
    { &hf_message_type,     { "Type",   "fractalgeneratorprotocol.message_type",     FT_UINT8,  BASE_DEC, VALS(message_type_values), 0x0, "", HFILL } },
    { &hf_message_flags,    { "Flags",  "fractalgeneratorprotocol.message_flags",    FT_UINT8,  BASE_DEC, NULL,                      0x0, "", HFILL } },
    { &hf_message_length,   { "Length", "fractalgeneratorprotocol.message_length",   FT_UINT16, BASE_DEC, NULL,                      0x0, "", HFILL } },
    { &hf_data_start_x,     { "StartX", "fractalgeneratorprotocol.data_start_x",     FT_UINT32, BASE_DEC, NULL,                      0x0, "", HFILL } },
    { &hf_data_start_y,     { "StartY", "fractalgeneratorprotocol.data_start_y",     FT_UINT32, BASE_DEC, NULL,                      0x0, "", HFILL } },
    { &hf_data_points,      { "Points", "fractalgeneratorprotocol.data_points",      FT_UINT32, BASE_DEC, NULL,                      0x0, "", HFILL } },
    { &hf_parameter_width,  { "Width",  "fractalgeneratorprotocol.parameter_width",  FT_UINT32, BASE_DEC, NULL,                      0x0, "", HFILL } },
    { &hf_parameter_height, { "Height", "fractalgeneratorprotocol.parameter_height", FT_UINT32, BASE_DEC, NULL,                      0x0, "", HFILL } },
    { &hf_parameter_maxiterations, { "MaxIterations", "fractalgeneratorprotocol.parameter_maxiterations", FT_UINT32, BASE_DEC, NULL,                      0x0, "", HFILL } },
    { &hf_parameter_algorithmid, { "AlgorithmID", "fractalgeneratorprotocol.parameter_algorithmid", FT_UINT32, BASE_DEC, NULL,                      0x0, "", HFILL } },
    { &hf_parameter_c1real, { "C1Real", "fractalgeneratorprotocol.parameter_c1real", FT_DOUBLE, BASE_DEC, NULL,                      0x0, "", HFILL } },
    { &hf_parameter_c1imag, { "C1Imag", "fractalgeneratorprotocol.parameter_c1imag", FT_DOUBLE, BASE_DEC, NULL,                      0x0, "", HFILL } },
    { &hf_parameter_c2real, { "C2Real", "fractalgeneratorprotocol.parameter_c2real", FT_DOUBLE, BASE_DEC, NULL,                      0x0, "", HFILL } },
    { &hf_parameter_c2imag, { "C2Imag", "fractalgeneratorprotocol.parameter_c2imag", FT_DOUBLE, BASE_DEC, NULL,                      0x0, "", HFILL } },
    { &hf_parameter_n, { "N", "fractalgeneratorprotocol.parameter_n", FT_DOUBLE, BASE_DEC, NULL,                      0x0, "", HFILL } },
  };

  /* Setup protocol subtree array */
  static gint *ett[] = {
    &ett_fractalgeneratorprotocol
  };

  /* Register the protocol name and description */
  proto_fractalgeneratorprotocol = proto_register_protocol("Fractal Generator Protocol", "FractalGeneratorProtocol",  "fractalgeneratorprotocol");

  /* Required function calls to register the header fields and subtrees used */
  proto_register_field_array(proto_fractalgeneratorprotocol, hf, array_length(hf));
  proto_register_subtree_array(ett, array_length(ett));

}

void
proto_reg_handoff_fractalgeneratorprotocol(void)
{
  dissector_handle_t fractalgeneratorprotocol_handle;

  fractalgeneratorprotocol_handle = create_dissector_handle(dissect_fractalgeneratorprotocol, proto_fractalgeneratorprotocol);
  dissector_add("sctp.ppi",  FRACTALGENERATORPROTOCOL_PAYLOAD_PROTOCOL_ID, fractalgeneratorprotocol_handle);
}

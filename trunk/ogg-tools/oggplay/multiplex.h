/* ogg bitstream (de)multiplexing */

typedef struct {
	int	(*can_handle) (ogg_packet *packet);
	int	(*packet_in) (ogg_packet *packet);
} om_callbacks;

typedef struct om_bitstream_s {
	int			serial_no;
	ogg_stream_state	*os;
	ogg_page		*last_page;
	ogg_packet		*last_packet;
	struct om_bitstream_s	*next;
	om_callbacks		*codec;
} om_bitstream;


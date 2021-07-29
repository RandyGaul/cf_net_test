#include <cute.h>
#include <time.h>
using namespace cute;

// This can be whatever you want. It's a unique identifier for your game or application, and
// used merely to identify yourself within Cute's packets. It's more of a formality than anything.
uint64_t g_application_id = 0;

// These keys were generated by the below function `print_embedded_keygen`. Usually it's a good idea
// to distribute your keys to your servers in absolute secrecy, perhaps keeping them hidden in a file
// and never shared publicly. But, putting them here is fine for testing purposes.

// Embedded g_public_key
int g_public_key_sz = 32;
unsigned char g_public_key_data[32] = {
	0x4a,0xc5,0x56,0x47,0x30,0xbf,0xdc,0x22,0xc7,0x67,0x3b,0x23,0xc5,0x00,0x21,0x7e,
	0x19,0x3e,0xa4,0xed,0xbc,0x0f,0x87,0x98,0x80,0xac,0x89,0x82,0x30,0xe9,0x95,0x6c
};
// Embedded g_secret_key
int g_secret_key_sz = 64;
unsigned char g_secret_key_data[64] = {
	0x10,0xaa,0x98,0xe0,0x10,0x5a,0x3e,0x63,0xe5,0xdf,0xa4,0xb5,0x5d,0xf3,0x3c,0x0a,
	0x31,0x5d,0x6e,0x58,0x1e,0xb8,0x5b,0xa4,0x4e,0xa3,0xf8,0xe7,0x55,0x53,0xaf,0x7a,
	0x4a,0xc5,0x56,0x47,0x30,0xbf,0xdc,0x22,0xc7,0x67,0x3b,0x23,0xc5,0x00,0x21,0x7e,
	0x19,0x3e,0xa4,0xed,0xbc,0x0f,0x87,0x98,0x80,0xac,0x89,0x82,0x30,0xe9,0x95,0x6c
};
CUTE_STATIC_ASSERT(sizeof(g_public_key_data) == sizeof(crypto_sign_public_t), "Must be equal.");
CUTE_STATIC_ASSERT(sizeof(g_secret_key_data) == sizeof(crypto_sign_secret_t), "Must be equal.");

void print_embedded_symbol(const char* sym, void* data, int sz)
{
	printf("// Embedded %s\n", sym);
	printf("int %s_sz = %d;\n", sym, sz);
	printf("unsigned char %s_data[%d] = {\n", sym, sz);

	const unsigned char* buf = (const unsigned char*)data;
	for (int i = 0; i < sz; ++i) {
		printf(
			i % 16 == 0 ? "\t0x%02x%s" : "0x%02x%s",
			buf[i],
			i == sz - 1 ? "" : (i + 1) % 16 == 0 ? ",\n" : ","
		);
	}
	printf("\n};\n");
}

void print_embedded_keygen()
{
	crypto_sign_public_t pk;
	crypto_sign_secret_t sk;
	crypto_sign_keygen(&pk, &sk);
	print_embedded_symbol("g_public_key", &pk, sizeof(pk));
	print_embedded_symbol("g_secret_key", &sk, sizeof(sk));
}

uint64_t unix_timestamp()
{
	time_t ltime;
	time(&ltime);
	tm* timeinfo = gmtime(&ltime);;
	return (uint64_t)mktime(timeinfo);
}

error_t make_test_connect_token(uint64_t unique_client_id, const char* address_and_port, uint8_t* connect_token_buffer)
{
	crypto_key_t client_to_server_key = crypto_generate_key();
	crypto_key_t server_to_client_key = crypto_generate_key();
	uint64_t current_timestamp = unix_timestamp();
	uint64_t expiration_timestamp = current_timestamp + 60; // Token expires in one minute.
	uint32_t handshake_timeout = 5;
	const char* endpoints[] = {
		address_and_port,
	};

	uint8_t user_data[CUTE_CONNECT_TOKEN_USER_DATA_SIZE];
	memset(user_data, 0, sizeof(user_data));

	error_t err = protocol::generate_connect_token(
		g_application_id,
		current_timestamp,
		&client_to_server_key,
		&server_to_client_key,
		expiration_timestamp,
		handshake_timeout,
		sizeof(endpoints) / sizeof(endpoints[0]),
		endpoints,
		unique_client_id,
		user_data,
		(crypto_sign_secret_t*)g_secret_key_data,
		connect_token_buffer
	);

	return err;
}

void panic(error_t err)
{
	printf("ERROR: %s\n", err.details);
	exit(-1);
}

int main(int argc, const char** argv)
{
	if (argc != 3) {
		printf("Incorrect command line. Here's an example for a client.\n\n");
		printf("\tclient [::1]:5000\n\n");
		printf("Here is an example for a server.\n\n");
		printf("\tserver [::1]:5000\n\n");
		exit(-1);
	}

	uint32_t options = CUTE_APP_OPTIONS_HIDDEN | CUTE_APP_OPTIONS_WINDOW_POS_CENTERED;
	app_t* app = app_make("Fancy Window Title", 0, 0, 640, 480, options, argv[0]);
	if (!app) {
		printf("Failed to initialize app.\n");
		exit(-1);
	}
	error_t err = app_init_net(app);
	if (err.is_error()) panic(err);

	bool is_server = argc > 1 && !strcmp("server", argv[1]);
	const char* address_and_port = argv[2];
	endpoint_t endpoint;
	endpoint_init(&endpoint, address_and_port);

	server_t* server = NULL;
	client_t* client = NULL;
	uint8_t connect_token[CUTE_CONNECT_TOKEN_SIZE];

	if (is_server) {
		server_config_t server_config;
		server_config.application_id = g_application_id;
		memcpy(server_config.public_key.key, g_public_key_data, sizeof(g_public_key_data));
		memcpy(server_config.secret_key.key, g_secret_key_data, sizeof(g_secret_key_data));

		server = server_create(&server_config);
		err = server_start(server, address_and_port);
		if (err.is_error()) panic(err);
		printf("Server started, listening on port %d.\n", (int)endpoint.port);
	} else {;
		client = client_make(endpoint.port, g_application_id);
		uint64_t client_id = rand() % 500; // Must be unique for each different player in your game.
		err = make_test_connect_token(client_id, address_and_port, connect_token);
		if (err.is_error()) panic(err);
		err = client_connect(client, connect_token);
		if (err.is_error()) panic(err);
		printf("Attempting to connect to server on port %d.\n", (int)endpoint.port);
	}
	
	printf("here1\n");
	while (app_is_running(app)) {
		printf("here aaah\n");
		float dt = calc_dt();
		printf("here2\n");
		uint64_t unix_time = unix_timestamp();
		printf("here3\n");
		app_update(app, dt);
		printf("here4\n");

		if (is_server) {
			server_update(server, (double)dt, unix_time);
			printf("here5\n");

			server_event_t e;
			while (server_pop_event(server, &e)) {
				if (e.type == SERVER_EVENT_TYPE_NEW_CONNECTION) {
					printf("New connection from id %d, on index %d.\n", (int)e.u.new_connection.client_id, e.u.new_connection.client_index);
				} else if (e.type == SERVER_EVENT_TYPE_PAYLOAD_PACKET) {
					printf("Got a message from client on index %d, \"%s\"\n", e.u.payload_packet.client_index, (const char*)e.u.payload_packet.data);
					server_free_packet(server, e.u.payload_packet.client_index, e.u.payload_packet.data);
				} else if (e.type == SERVER_EVENT_TYPE_DISCONNECTED) {
					printf("Client disconnected on index %d.\n", e.u.disconnected.client_index);
				}
			}
		} else {
			client_update(client, (double)dt, unix_time);

			if (client_state_get(client) == CLIENT_STATE_CONNECTED) {
				static bool notify = false;
				if (!notify) {
					notify = true;
					printf("Connected! Press ESC to gracefully disconnect.\n");
				}

				static float t = 0;
				t += dt;
				if (t > 2) {
					const char* data = "What's up over there, Mr. Server?";
					int size = (int)strlen(data) + 1;
					client_send(client, data, size, false);
					t = 0;
				}

				if (key_was_pressed(app, KEY_ESCAPE)) {
					client_disconnect(client);
					app_stop_running(app);
				}
			} else if (client_state_get(client) < 0) {
				printf("Client encountered an error: %s.\n", client_state_string(client_state_get(client)));
				exit(-1);
			}
		}
	}

	app_destroy(app);

	return 0;
}

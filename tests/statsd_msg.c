#include <string.h>

#include "sput.h"
#include "brubeck.h"

static void try_parse(struct brubeck_statsd_msg *msg, const char *msg_text, double expected)
{
	char buffer[64];
	strcpy(buffer, msg_text);

	sput_fail_unless(brubeck_statsd_msg_parse(msg, buffer) == 0, msg_text);
	sput_fail_unless(expected == msg->value.n, "msg.value.n == expected");
}

static void try_parse_set(struct brubeck_statsd_msg *msg, const char *msg_text, const char *expected)
{
	char buffer[64];
	strcpy(buffer, msg_text);

	sput_fail_unless(brubeck_statsd_msg_parse(msg, buffer) == 0, msg_text);
	sput_fail_unless(0 == strcmp(msg->value.s, expected), "msg.value.s == expected");
}

static void try_split(struct brubeck_sampler *sampler, const char *input, int expected)
{
	char buffer[512];
	strcpy(buffer, input);
	int len = strlen(input);
	struct in_addr src;

	sput_fail_unless(brubeck_statsd_split_buffer(sampler, buffer, len, &src) == expected,
			"brubeck_statsd_split_buffer() == expected");
}

static struct { char *metric; double expected; } test_values[] = {
	{ "github.auth.fingerprint.sha1:1|c", 1 },
	{ "github.auth.fingerprint.sha1:1|c|@args", 1 },
	{ "github.auth.fingerprint.sha1:1|g", 1 },
	{ "lol:1|ms", 1 },
	{ "this.is.sparta:199812|C", 199812 },
	{ "this.is.sparta:0012|h", 12 },
	{ "this.is.sparta:23.23|g", 23.23 },
	{ "this.is.sparta:0.232030|g", 0.23203 },
	{ "this.are.some.floats:1234567.89|g", 1234567.89 },
	{ NULL, 0 },
};

void test_statsd_msg__parse_strings(void)
{
	struct brubeck_statsd_msg msg;
	int i;

	for(i = 0; test_values[i].metric != NULL; i++) {
		try_parse(&msg, test_values[i].metric, test_values[i].expected);
	}

#define EXPECT "some.non-float.value"
	try_parse_set(&msg, "this.is.a.set:" EXPECT "|s|@args", EXPECT);
	try_parse_set(&msg, "this.is.a.set:" EXPECT "|s", EXPECT);
}

void test_statsd_msg__split_buffer(void)
{
	struct brubeck_sampler sampler;
	struct brubeck_server server = {};
	int i;

	server.at_capacity = true;	// don't try to allocate
	server.metrics = brubeck_hashtable_new(1 << 1);
	sampler.server = &server;
	sampler.type = BRUBECK_SAMPLER_STATSD;

	for(i = 0; test_values[i].metric != NULL; i++) {
		try_split(&sampler, test_values[i].metric, 1);
	}

	try_split(&sampler,
		"missing.metric.type:0\n"
		"ab.cd.ef:0|g\n"
		"missing.metric.value|g\n"
		"ghi.jkl.mnop:-1|g"
		, 2);
	try_split(&sampler,
		"missing.metric.type:0@trail\n"
		"ab.cd.ef:0|g@foo\n"
		"missing.metric.value|g\n"
		"x.y.z:-1|g\n"
		, 2);
}

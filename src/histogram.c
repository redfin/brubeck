#include "brubeck.h"

#define HISTO_INIT_SIZE 16

void brubeck_histo_push(struct brubeck_histo *histo, value_t value, value_t sample_rate)
{
	if (histo->size == histo->alloc) {
		histo->alloc *= 2;
		if (histo->alloc < HISTO_INIT_SIZE)
			histo->alloc = HISTO_INIT_SIZE;

		histo->values = xrealloc(histo->values, histo->alloc * sizeof(*histo->values));
	}

	histo->values[histo->size  ].value = value;
	histo->values[histo->size++].count = 1.0 / sample_rate;
}

static inline value_t histo_percentile(struct brubeck_histo *histo, value_t total, value_t rank)
{
	size_t i;
	rank *= total;
	for(i = histo->size - 1; i >= 0; i--) {
		total -= histo->values[i].count;
		if(total < rank)
			return histo->values[i].value;
	}
	return histo->values[0].value;
}

static inline value_t histo_sum(struct brubeck_histo *histo, value_t *pcount)
{
	size_t i;
	value_t sum = 0.0;
	value_t count = 0.0;

	for (i = 0; i < histo->size; ++i) {
		sum += histo->values[i].value * histo->values[i].count;
		count += histo->values[i].count;
	}

	*pcount = count;
	return sum;
}

static int value_cmp(const void *a, const void *b)
{
	value_t va = ((struct brubeck_histo_scaled_value *)a)->value;
	value_t vb = ((struct brubeck_histo_scaled_value *)b)->value;

	if (va < vb)
		return -1;
	if (va > vb)
		return 1;
	return 0;
}

static inline void histo_sort(struct brubeck_histo *histo)
{
	qsort(histo->values, histo->size, sizeof(*histo->values), &value_cmp);
}

void brubeck_histo_sample(
		struct brubeck_histo_sample *sample,
		struct brubeck_histo *histo)
{
	if (histo->size == 0) {
		memset(sample, 0x0, sizeof(struct brubeck_histo_sample));
		return;
	}

	histo_sort(histo);

	sample->sum = histo_sum(histo, &sample->count);
	sample->min = histo->values[0].value;
	sample->max = histo->values[histo->size - 1].value;
	sample->mean = sample->sum / sample->count;
	sample->median = histo_percentile(histo, sample->count, 0.5);

	sample->percentile[PC_75] = histo_percentile(histo, sample->count, 0.75);
	sample->percentile[PC_95] = histo_percentile(histo, sample->count, 0.95);
	sample->percentile[PC_98] = histo_percentile(histo, sample->count, 0.98);
	sample->percentile[PC_99] = histo_percentile(histo, sample->count, 0.99);
	sample->percentile[PC_999] = histo_percentile(histo, sample->count, 0.999);

	/* empty the histogram */
	histo->size = 0;
}

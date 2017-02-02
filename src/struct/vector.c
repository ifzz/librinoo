/**
 * @file   vector.c
 * @author Reginald Lips <reginald.@gmail.com> - Copyright 2014
 * @date   Wed Feb  1 18:56:27 2017
 *
 * @brief  Vector functions
 *
 *
 */

#include "rinoo/struct/module.h"

/**
 * Adds an element to a vector
 *
 * @param vector Vector to add the element to
 * @param item Item to add to the vector
 *
 * @return 0 on success, otherwise -1
 */
int rn_vector_add(rn_vector_t *vector, void *item)
{
	void *ptr;
	size_t msize;

	if (vector == NULL) {
		return -1;
	}
	if (vector->size == vector->msize) {
		msize = vector->msize * 2;
		if (msize > 0) {
			ptr = realloc(vector->ptr, sizeof(*vector->ptr) * msize);
		} else {
			msize = 8;
			ptr = malloc(sizeof(*vector->ptr) * msize);
		}
		if (ptr == NULL) {
			return -1;
		}
		vector->ptr = ptr;
		vector->msize = msize;
	}
	vector->ptr[vector->size] = item;
	vector->size++;
	return 0;
}

/**
 * Destroys a vector
 *
 * @param vector Vector to destroy
 */
void rn_vector_destroy(rn_vector_t *vector)
{
	if (vector == NULL || vector->msize == 0 || vector->ptr == NULL) {
		return;
	}
	free(vector->ptr);
	vector->ptr = NULL;
}

/**
 * Removes an element from a vector
 *
 * @param vector Vector where to remove the element from
 * @param i Index of the element to remove
 *
 * @return 0 on success, otherwise -1
 */
int rn_vector_remove(rn_vector_t *vector, uint32_t i)
{
	if (vector == NULL || i >= vector->size) {
		return -1;
	}
	if (i < vector->size - 1) {
		memmove(&vector->ptr[i], &vector->ptr[i + 1], sizeof(*vector->ptr) * (vector->size - i - 1));
	}
	vector->size--;
	return 0;
}

/**
 * Gets an element from a vector
 *
 * @param vector Vector where to get the element from
 * @param i Index of the element to retrieve
 *
 * @return Pointer to the element on success, otherwise NULL
 */
void *rn_vector_get(rn_vector_t *vector, uint32_t i)
{
	if (vector == NULL || i >= vector->size) {
		return NULL;
	}
	return vector->ptr[i];
}

/**
 * Gets vector size
 *
 * @param vector Pointer to a vector
 *
 * @return Size of the vector
 */
size_t rn_vector_size(rn_vector_t *vector)
{
	if (vector == NULL) {
		return 0;
	}
	return vector->size;
}

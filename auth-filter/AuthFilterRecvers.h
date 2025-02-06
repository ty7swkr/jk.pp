/*
 * AuthFilterRecvers.h
 *
 *  Created on: 2024. 12. 4.
 *      or: tys
 */

#pragma once

#include <NatsRecvers.h>
#include "AuthFilterWorkerPool.h"

using AuthFilterRecvers = NatsRecvers<AuthFilterWorkerPool>;


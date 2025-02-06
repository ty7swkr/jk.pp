/*
 * AuthFilterWorkerPool.h
 *
 *  Created on: 2024. 12. 4.
 *      or: tys
 */

#pragma once

#include "AuthFilterWorker.h"
#include <WorkerPool.h>

using AuthFilterWorkerPool = WorkerPool<AuthFilterWorker>;


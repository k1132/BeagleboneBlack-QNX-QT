/*
 * $QNXLicenseC:
 * Copyright 2010, QNX Software Systems.
 * Copyright 2017 Varghese Alphonse
 *
 * Licensed under the Apache License, Version 2.0 (the "License"). You
 * may not reproduce, modify or distribute this software except in
 * compliance with the License. You may obtain a copy of the License
 * at: http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTIES OF ANY KIND, either express or implied.
 *
 * This file may contain contributions from others, either as
 * contributors under the License or as licensors under other terms.
 * Please review this entire file for other proprietary rights or license
 * notices, as well as the QNX Development Suite License Guide at
 * http://licensing.qnx.com/license-guide/ for other information.
 * $
 */

#include <stdlib.h>
#include "omapl1xx.h"


static int
omapl1xx_mem_init(disp_adapter_t *adapter, char *optstring)
{
	;
	//return 0;
}

static void
omapl1xx_mem_fini(disp_adapter_t *adapter)
{
	return;
}

static int
omapl1xx_mem_reset(disp_adapter_t *adapter, disp_surface_t *surf)
{
	disp_surface_t			vram;
	int			stride;
	slogf(_SLOGC_GRAPHICS, _SLOG_INFO, "omapl1xx_mem_reset");
	register omapl1xx_context_t	*omapl1xx = adapter->ms_ctx;

	vram.stride = vram.width = omapl1xx->vidsize;//- 1024;
	vram.height = 1;
//	vram.pixel_format = DISP_SURFACE_FORMAT_BYTES;
	vram.pixel_format = DISP_SURFACE_FORMAT_RGB565;
	vram.vidptr = omapl1xx->vidptr;
	vram.paddr = omapl1xx->vidbase;
	vram.offset = 0;
	vram.flags = DISP_SURFACE_DISPLAYABLE | DISP_SURFACE_SCALER_DISPLAYABLE |
	    DISP_SURFACE_CPU_LINEAR_READABLE |
	    DISP_SURFACE_CPU_LINEAR_WRITEABLE |
	    DISP_SURFACE_2D_TARGETABLE | DISP_SURFACE_2D_READABLE |
	    DISP_SURFACE_PHYS_CONTIG;
	stride=omapl1xx->width*omapl1xx->height*2;
	adapter->mm_ctx = disp_getmem(stride, PROT_READ|PROT_WRITE,
			vram.flags);
	return 0;
}


static disp_surface_t *
omapl1xx_alloc_surface(disp_adapter_t *adapter, int width, int height, unsigned format, unsigned flags, unsigned user_flags)
{
	omapl1xx_context_t		*vb = adapter->ms_ctx;
		int			stride;
		disp_surface_t		*surf;
		unsigned		mapflags = 0;

		if (adapter->mm_ctx == NULL)
			return NULL;
		if(format == DISP_SURFACE_FORMAT_RGB565)
			stride = (width*DISP_BYTES_PER_PIXEL(format) + 31) & ~31;
	    else
	    	stride = (width*DISP_BYTES_PER_PIXEL(format) + 31) & ~31;
		if ((surf = calloc(1, sizeof (*surf))) == NULL)
				return NULL;
		if (flags & DISP_SURFACE_PHYS_CONTIG)
				mapflags |= DISP_MAP_PHYS;

		surf->size = sizeof(*surf);
		surf->stride = stride;
		surf->width = width;
		surf->height = height;
		surf->flags = flags |DISP_SURFACE_CPU_LINEAR_WRITEABLE|DISP_SURFACE_CPU_LINEAR_READABLE;
		surf->pixel_format = format;
		surf->vidptr =adapter->mm_ctx;

		surf->paddr = disp_phys_addr(adapter->mm_ctx);
		if(surf->paddr==NULL)
			slogf(_SLOGC_GRAPHICS, _SLOG_INFO, "at91sam9xx_free_surface");
		return surf;
}
/*
static disp_surface_t *
omapl1xx_alloc_layer_surface(disp_adapter_t *adapter, int dispno[],
    int layer_idx[], int nlayers, unsigned format, int surface_index,
    int width, int height, unsigned sflags, unsigned hint_flags)
{
	return NULL;
}*/

static int
omapl1xx_free_surface(disp_adapter_t *adapter, disp_surface_t *surf)
{
	disp_freemem(surf->paddr,surf->stride );
	return 0;
}

static unsigned long
omapl1xx_mem_avail(disp_adapter_t *adapter, unsigned flags)
{
	if (adapter->mm_ctx)
			return disp_vm_mem_avail(adapter->mm_ctx);
	return 0;
}

static int
omapl1xx_query_apertures(disp_adapter_t *adp, disp_aperture_t *ap)
{
	omapl1xx_context_t	*omapl1xx = adp->ms_ctx;

    ap->base = omapl1xx->vidbase;
	ap->size = omapl1xx->vidsize;

	ap->flags = DISP_APER_NOCACHE;
	return 1;
}

static int
omapl1xx_query_surface(disp_adapter_t *adapter,
    disp_surface_t *surf, disp_surface_info_t *info)
{
	 info->aperture_index = 0;
	        info->offset = surf->offset;
	return 0;
}

/*
 * If a client of the driver wants to allocate memory itself,
 * it must allocate it in accordance with the parameters returned by
 * this function.  Since this memory will not be coming from
 * video memory, we must check the flags accordingly.
 */
static int
omapl1xx_get_alloc_info(disp_adapter_t *adapter,
    int width, int height, unsigned format,
    unsigned flags, unsigned user_flags, disp_alloc_info_t *info)
{
	   flags &= DISP_SURFACE_CAPS_MASK;
	        if (flags & ~(DISP_SURFACE_CPU_LINEAR_READABLE |
	            DISP_SURFACE_CPU_LINEAR_WRITEABLE| DISP_SURFACE_ALPHA_MAP|
	            DISP_SURFACE_PAGE_ALIGNED|DISP_SURFACE_PHYS_CONTIG))
	                return -1;

	        info->start_align = 4;
	        info->end_align = 1;
	        info->min_stride = width * DISP_BYTES_PER_PIXEL(format);
	        info->max_stride = ~0;
	        info->stride_gran = 4;
	        info->map_flags = 0;
	        info->prot_flags = DISP_PROT_READ | DISP_PROT_WRITE;
	        info->surface_flags = DISP_SURFACE_CPU_LINEAR_READABLE |
	            DISP_SURFACE_CPU_LINEAR_WRITEABLE | DISP_SURFACE_DRIVER_NOT_OWNER;

	return 0;
}

static int
omapl1xx_get_alloc_layer_info(disp_adapter_t *adapter, int dispno[], int layer_idx[],
    int nlayers, unsigned format, int surface_index, int width, int height,
    unsigned sflags, unsigned hint_flags, disp_alloc_info_t *info)
{
    return -1;
}


int
devg_get_memfuncs(disp_adapter_t *adapter, disp_memfuncs_t *funcs, int tabsize)
{
	DISP_ADD_FUNC(disp_memfuncs_t, funcs, init, omapl1xx_mem_init, tabsize);
	DISP_ADD_FUNC(disp_memfuncs_t, funcs, fini, omapl1xx_mem_fini, tabsize);
	DISP_ADD_FUNC(disp_memfuncs_t, funcs, module_info, omapl1xx_module_info, tabsize);
	DISP_ADD_FUNC(disp_memfuncs_t, funcs, reset, omapl1xx_mem_reset, tabsize);
	DISP_ADD_FUNC(disp_memfuncs_t, funcs, alloc_surface, omapl1xx_alloc_surface, tabsize);
//	DISP_ADD_FUNC(disp_memfuncs_t, funcs, alloc_layer_surface, omapl1xx_alloc_layer_surface, tabsize);
	DISP_ADD_FUNC(disp_memfuncs_t, funcs, free_surface, omapl1xx_free_surface, tabsize);
	DISP_ADD_FUNC(disp_memfuncs_t, funcs, mem_avail, omapl1xx_mem_avail, tabsize);
	DISP_ADD_FUNC(disp_memfuncs_t, funcs, query_apertures, omapl1xx_query_apertures, tabsize);
	DISP_ADD_FUNC(disp_memfuncs_t, funcs, query_surface, omapl1xx_query_surface, tabsize);
	DISP_ADD_FUNC(disp_memfuncs_t, funcs, get_alloc_info, omapl1xx_get_alloc_info, tabsize);
	DISP_ADD_FUNC(disp_memfuncs_t, funcs, get_alloc_layer_info, omapl1xx_get_alloc_layer_info, tabsize);

	return 0;
}


..  Copyright (C) 2021 Authors of Cilium

===================================
Tail Call Hooks for Custom Programs
===================================

The files in this directory implement a sample byte counter program which can
be attached to dedicated hooks in the datapath.

.. note::
    This is a beta feature. Please provide feedback and file a GitHub issue if
    you experience any problems.

Datapath Hooks
==============

Implementation
--------------

When compiled with ``ENABLE_CUSTOM_CALLS`` set, Cilium's datapath contains some
tail call hooks permitting to insert custom programs in the datapath. These
custom programs should be particularly careful *not* to interfere with the
logics of the datapath, at the risk of altering or breaking the network
connectivity in the cluster.

Before the tail call happens, the datapath encodes two elements into the ``cb``
field of the socket buffer for the packet:

- The return value for that packet, so that the custom program can return it on
  exit. This is because a tail call is not like a function call, and the custom
  program does not *return* into the core datapath logics. Therefore, the
  custom program must be called as the very last step of the processing of the
  packet, and must preserve the return value selected by the datapath.

- The identity corresponding to the packet, that is, the Cilium identity
  associated to the sender for the ingress hook, and to the receiver when on
  the egress hook. This identity is not strictly necessary to the custom
  program, but it prove useful for some use cases. Given that the datapath
  already did the work for retrieving the identity for the packet, it just
  passes it down to the custom programs, in case it saves them the trouble to
  extract it a second time.

Limitations
-----------

- There is currently no hook on the egress path for socket-based
  load-balancing.

Sample Byte Counter Program
============================

This directory contains a sample byte counter program, to help count how many
bytes a given endpoint sends or receives, and to understand how the tail call
hooks work.

The following steps indicate how to use this example program.

1. Compile the program. This should be easy thanks to the provided Makefile.
   From the root of the Cilium repository::

       $ make -C bpf/custom

2. Load and pin the byte counter program. Bpftool can do it::

       # bpftool prog load bpf/custom/bpf_custom.o \
           /sys/fs/bpf/tc/globals/bytecounter \
           type classifier \
           pinmaps /sys/fs/bpf/tc/globals/bytecounter_maps

3. Pick the endpoint for which the program should count the bytes. Update the
   tail call map for custom programs for that endpoint, with a reference to the
   pinned byte counter program::

       $ EP=<endpoint_id>
       # bpftool map update \
           pinned /sys/fs/bpf/tc/globals/cilium_calls_custom_$(printf '%05d' ${EP}) \
           key 0 0 0 0 \
           value pinned /sys/fs/bpf/tc/globals/bytecounter

   The key indicate the hook to use. It is possible to attach the program at
   multiple hooks, although the counter currently makes no distinction and will
   condensate the values collected from all hooks into a single counter. The
   available hooks and their keys are the following:

   - IPv4, ingress: ``key 0 0 0 0``
   - IPv4, egress:  ``key 1 0 0 0``
   - IPv6, ingress: ``key 2 0 0 0``
   - IPv6, egress:  ``key 3 0 0 0``

4. After some traffic has flown, dump the content of the byte counter map to
   read the statistics::

       # bpftool map dump \
           pinned /sys/fs/bpf/tc/globals/bytecounter_maps/bytecount_map

5. When the byte counter is no longer neccessary, it is possible to clean it up
   by deleting the entry in the tail call map, and removing the pinned
   objects::

       # bpftool map delete \
           pinned /sys/fs/bpf/tc/globals/cilium_calls_custom_$(printf '%05d' ${EP}) \
           key 0 0 0 0
       # rm /sys/fs/bpf/tc/globals/custom_prog
       # rm -r /sys/fs/bpf/tc/globals/bytecounter_maps

Using Other Custom Programs
===========================

Other custom programs can be hooked into the datapath, just like the byte
counter program. The workflow would be similar:

1. Write a custom program, and make sure it does not interfere with the logics
   of the datapath. It is recommended to define a custom function and to
   include it in the “landing point” program from bpf_custom.c.

2. Compile your program. If reusing bpf_custom.c, this is easy to do. Just pass
   the name of the file containing the ``custom_prog`` function when calling
   make::

       $ BPF_CUSTOM_PROG=custom_function.h make -C bpf/custom

3. Load and pin the custom program::

       # bpftool prog load bpf/custom/bpf_custom.o \
           /sys/fs/bpf/tc/globals/custom_prog \
           type classifier

4. Select your endpoint and update its custom tail call map with a reference to
   the custom program, for the desired hook (direction and IP version). The
   command below is for ingress on IPv4, for the endpoint with id 1234::

       # bpftool map update \
           pinned /sys/fs/bpf/tc/globals/cilium_calls_custom_01234 \
           key 0 0 0 0 \
           value pinned /sys/fs/bpf/tc/globals/custom_prog

5. The program should run for each packet. If it uses maps to share data with
   userspace, it should now be possible to retrieve the statistics.

6. Clean up by deleting the entry in the tail call map and removing all pinned
   objects in the eBPF virtual file system::

       # bpftool map delete \
           pinned /sys/fs/bpf/tc/globals/cilium_calls_custom_01234 \
           key 0 0 0 0
       # rm /sys/fs/bpf/tc/globals/custom_prog

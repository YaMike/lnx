
main:     file format elf32-littlearc

Disassembly of section .init:

00010264 <_init-0x4>:
   10264:	00 00 00 00             	           

00010268 <_init>:
   10268:	f1 c0                   	push_s     blink
   1026a:	e0 78                   	nop_s      
   1026c:	d2 08 00 00             	bl         1033c <frame_dummy>

   10270:	4a 0a 00 00             	bl         104b8 <__do_global_ctors_aux>

   10274:	d1 c0                   	pop_s      blink
   10276:	e0 7e                   	j_s [blink] 
Disassembly of section .plt:

00010278 <.plt>:
   10278:	00 16 0b 70 01 00 24 26 	ld         r11,[0x00012624]
   10280:	00 16 0a 70 01 00 28 26 	ld         r10,[0x00012628]
   10288:	20 20 80 02             	j          [r10]
   1028c:	20 26 01 00             	j          [r0]
   10290:	30 27 8c 7f 00 00 9c 23 	ld         r12,[pcl,0x239c]
   10298:	20 7c                   	j_s.d      [r12]
   1029a:	ef 74                   	mov_s      r12,pcl
   1029c:	30 27 8c 7f 00 00 94 23 	ld         r12,[pcl,0x2394]
   102a4:	20 7c                   	j_s.d      [r12]
   102a6:	ef 74                   	mov_s      r12,pcl
Disassembly of section .text:

000102a8 <__start>:
   102a8:	4a 23 00 30             	mov        fp,0
   102ac:	00 c1                   	ld_s       r1,[sp,0]
   102ae:	b8 70                   	mov_s      r5,r0
   102b0:	81 c2                   	add_s      r2,sp,4
   102b2:	cf 70 01 00 78 03       	mov_s      r0,0x00010378
   102b8:	cf 73 01 00 68 02       	mov_s      r3,0x00010268
   102be:	0a 24 80 0f 01 00 00 05 	mov        r4,0x00010500
   102c6:	84 24 3f 3e             	and        sp,sp,-8
   102ca:	0a 26 00 07             	mov        r6,sp
   102ce:	d2 0f cf ff             	bl         1029c <_init+0x34>

   102d2:	07 00 00 00             	b          102d6 <_exit_halt>


000102d6 <_exit_halt>:
   102d6:	69 20 40 00             	flag       1
   102da:	4a 26 00 70             	nop        
   102de:	4a 26 00 70             	nop        
   102e2:	4a 26 00 70             	nop        
   102e6:	f3 07 cf ff             	b          102d6 <_exit_halt>

   102ea:	4a 26 00 70             	nop        
	...

000102f0 <__do_global_dtors_aux>:
   102f0:	f1 c0                   	push_s     blink
   102f2:	fc 1c c8 b6             	st.a       fp,[sp,-4]
   102f6:	0a 23 00 37             	mov        fp,sp
   102fa:	00 16 82 70 01 00 34 26 	ldb        r2,[0x00012634]
   10302:	0b ea                   	breq_s     r2,0,10316 <__do_global_dtors_aux+0x26>

   10304:	29 00 00 00             	b          1032c <__do_global_dtors_aux+0x3c>

   10308:	44 6a                   	add_s      r2,r2,4
   1030a:	00 1e 80 70 01 00 70 25 	st         r2,[0x00012570]
   10312:	22 20 c0 00             	jl         [r3]
   10316:	00 16 02 70 01 00 70 25 	ld         r2,[0x00012570]
   1031e:	60 82                   	ld_s       r3,[r2,0]
   10320:	f4 eb                   	brne_s     r3,0,10308 <__do_global_dtors_aux+0x18>

   10322:	01 da                   	mov_s      r2,1
   10324:	00 1e 82 70 01 00 34 26 	stb        r2,[0x00012634]
   1032c:	04 14 1b 34             	ld.ab      fp,[sp,4]
   10330:	00 14 1f 32             	ld.a       blink,[sp,0]
   10334:	e0 7f                   	j_s.d [blink] 
   10336:	40 24 1c 31             	add        sp,sp,4
   1033a:	e0 78                   	nop_s      

0001033c <frame_dummy>:
   1033c:	f8 1c c8 b6             	st.a       fp,[sp,-8]
   10340:	0a 23 00 37             	mov        fp,sp
   10344:	00 16 02 70 01 00 1c 26 	ld         r2,[0x0001261c]
   1034c:	4b 7a                   	tst_s      r2,r2
   1034e:	20 00 01 00             	bz         1036c <frame_dummy+0x30>

   10352:	cf 72 00 00 00 00       	mov_s      r2,0
   10358:	4b 7a                   	tst_s      r2,r2
   1035a:	14 00 01 00             	bz         1036c <frame_dummy+0x30>

   1035e:	cf 70 01 00 1c 26       	mov_s      r0,0x0001261c
   10364:	04 14 1b 34             	ld.ab      fp,[sp,4]
   10368:	a1 c0                   	add_s      sp,sp,4
   1036a:	00 7a                   	j_s        [r2]
   1036c:	04 14 1b 34             	ld.ab      fp,[sp,4]
   10370:	e0 7f                   	j_s.d [blink] 
   10372:	40 24 1c 31             	add        sp,sp,4
   10376:	e0 78                   	nop_s      

00010378 <main>:
   10378:	f1 c0                   	push_s     blink
   1037a:	fc 1c c8 b6             	st.a       fp,[sp,-4]
   1037e:	0a 23 00 37             	mov        fp,sp
   10382:	a6 c1                   	sub_s      sp,sp,24
   10384:	f0 1b 00 b0             	st         r0,[fp,-16]
   10388:	ec 1b 40 b0             	st         r1,[fp,-20]
   1038c:	cf 72 01 00 38 26       	mov_s      r2,0x00012638
   10392:	f4 1b 80 b0             	st         r2,[fp,-12]
   10396:	f4 13 02 b0             	ld         r2,[fp,-12]
   1039a:	40 82                   	ld_s       r2,[r2,0]
   1039c:	cf 70 01 00 18 05       	mov_s      r0,0x00010518
   103a2:	48 71                   	mov_s      r1,r2
   103a4:	ee 0e cf ff             	bl         10290 <_init+0x28>

   103a8:	f4 13 03 b0             	ld         r3,[fp,-12]
   103ac:	01 da                   	mov_s      r2,1
   103ae:	40 a3                   	st_s       r2,[r3,0]
   103b0:	f4 13 03 b0             	ld         r3,[fp,-12]
   103b4:	00 da                   	mov_s      r2,0
   103b6:	41 a3                   	st_s       r2,[r3,4]
   103b8:	f4 13 03 b0             	ld         r3,[fp,-12]
   103bc:	01 da                   	mov_s      r2,1
   103be:	42 a3                   	st_s       r2,[r3,8]
   103c0:	f4 13 03 b0             	ld         r3,[fp,-12]
   103c4:	02 da                   	mov_s      r2,2
   103c6:	43 a3                   	st_s       r2,[r3,12]
   103c8:	f4 13 03 b0             	ld         r3,[fp,-12]
   103cc:	03 da                   	mov_s      r2,3
   103ce:	44 a3                   	st_s       r2,[r3,16]
   103d0:	f4 13 03 b0             	ld         r3,[fp,-12]
   103d4:	04 da                   	mov_s      r2,4
   103d6:	45 a3                   	st_s       r2,[r3,20]
   103d8:	f4 13 03 b0             	ld         r3,[fp,-12]
   103dc:	05 da                   	mov_s      r2,5
   103de:	46 a3                   	st_s       r2,[r3,24]
   103e0:	f4 13 03 b0             	ld         r3,[fp,-12]
   103e4:	06 da                   	mov_s      r2,6
   103e6:	47 a3                   	st_s       r2,[r3,28]
   103e8:	f4 13 03 b0             	ld         r3,[fp,-12]
   103ec:	07 da                   	mov_s      r2,7
   103ee:	48 a3                   	st_s       r2,[r3,32]
   103f0:	f4 13 03 b0             	ld         r3,[fp,-12]
   103f4:	08 da                   	mov_s      r2,8
   103f6:	49 a3                   	st_s       r2,[r3,36]
   103f8:	f4 13 03 b0             	ld         r3,[fp,-12]
   103fc:	09 da                   	mov_s      r2,9
   103fe:	4a a3                   	st_s       r2,[r3,40]
   10400:	f4 13 02 b0             	ld         r2,[fp,-12]
   10404:	40 82                   	ld_s       r2,[r2,0]
   10406:	cf 70 01 00 18 05       	mov_s      r0,0x00010518
   1040c:	48 71                   	mov_s      r1,r2
   1040e:	86 0e cf ff             	bl         10290 <_init+0x28>

   10412:	00 da                   	mov_s      r2,0
   10414:	f8 1b 80 b0             	st         r2,[fp,-8]
   10418:	2b 00 00 00             	b          10442 <main+0xca>

   1041c:	f8 13 02 b0             	ld         r2,[fp,-8]
   10420:	f4 13 03 b0             	ld         r3,[fp,-12]
   10424:	41 6a                   	add_s      r2,r2,1
   10426:	f0 23 82 00             	ld.as      r2,[r3,r2]
   1042a:	cf 70 01 00 24 05       	mov_s      r0,0x00010524
   10430:	f8 13 01 b0             	ld         r1,[fp,-8]
   10434:	5e 0e cf ff             	bl         10290 <_init+0x28>

   10438:	f8 13 02 b0             	ld         r2,[fp,-8]
   1043c:	41 6a                   	add_s      r2,r2,1
   1043e:	f8 1b 80 b0             	st         r2,[fp,-8]
   10442:	f8 13 02 b0             	ld         r2,[fp,-8]
   10446:	89 e2                   	cmp_s      r2,9
   10448:	d4 07 cc ff             	ble        1041c <main+0xa4>

   1044c:	00 da                   	mov_s      r2,0
   1044e:	fc 1b 80 b0             	st         r2,[fp,-4]
   10452:	f4 13 02 b0             	ld         r2,[fp,-12]
   10456:	40 82                   	ld_s       r2,[r2,0]
   10458:	cf 70 01 00 30 05       	mov_s      r0,0x00010530
   1045e:	48 71                   	mov_s      r1,r2
   10460:	32 0e cf ff             	bl         10290 <_init+0x28>

   10464:	cf 70 01 00 38 26       	mov_s      r0,0x00012638
   1046a:	42 08 00 00             	bl         104a8 <set_bit_test>

   1046e:	fc 1b 00 b0             	st         r0,[fp,-4]
   10472:	cf 70 01 00 44 05       	mov_s      r0,0x00010544
   10478:	fc 13 01 b0             	ld         r1,[fp,-4]
   1047c:	16 0e cf ff             	bl         10290 <_init+0x28>

   10480:	f4 13 02 b0             	ld         r2,[fp,-12]
   10484:	40 82                   	ld_s       r2,[r2,0]
   10486:	cf 70 01 00 58 05       	mov_s      r0,0x00010558
   1048c:	48 71                   	mov_s      r1,r2
   1048e:	06 0e cf ff             	bl         10290 <_init+0x28>

   10492:	00 da                   	mov_s      r2,0
   10494:	48 70                   	mov_s      r0,r2
   10496:	a6 c0                   	add_s      sp,sp,24
   10498:	04 14 1b 34             	ld.ab      fp,[sp,4]
   1049c:	00 14 1f 32             	ld.a       blink,[sp,0]
   104a0:	e0 7f                   	j_s.d [blink] 
   104a2:	40 24 1c 31             	add        sp,sp,4
   104a6:	e0 78                   	nop_s      

000104a8 <set_bit_test>:
   104a8:	e1 c1                   	push_s     r1
   104aa:	20 80                   	ld_s       r1,[r0,0]
   104ac:	0a d9                   	mov_s      r1,10
   104ae:	20 a0                   	st_s       r1,[r0,0]
   104b0:	28 70                   	mov_s      r0,r1
   104b2:	c1 c1                   	pop_s      r1
   104b4:	20 20 c0 07             	j          [blink]

000104b8 <__do_global_ctors_aux>:
   104b8:	f1 c0                   	push_s     blink
   104ba:	f8 1c 48 b3             	st.a       r13,[sp,-8]
   104be:	fc 1c c8 b6             	st.a       fp,[sp,-4]
   104c2:	0a 23 00 37             	mov        fp,sp
   104c6:	00 16 02 70 01 00 0c 26 	ld         r2,[0x0001260c]
   104ce:	cf 75 01 00 0c 26       	mov_s      r13,0x0001260c
   104d4:	19 0a 80 0f ff ff ff ff 	breq       r2,-1,104ec <__do_global_ctors_aux+0x34>

   104dc:	22 20 80 00             	jl         [r2]
   104e0:	fc 15 02 92             	ld.a       r2,[r13,-4]
   104e4:	f9 0a 81 8f ff ff ff ff 	brne       r2,-1,104dc <__do_global_ctors_aux+0x24>

   104ec:	04 14 1b 34             	ld.ab      fp,[sp,4]
   104f0:	02 14 1f 36             	ld.as      blink,[sp,2]
   104f4:	00 c5                   	ld_s       r13,[sp,0]
   104f6:	e0 7f                   	j_s.d [blink] 
   104f8:	40 24 1c 33             	add        sp,sp,12
Disassembly of section .fini:

000104fc <_fini-0x4>:
   104fc:	00 00 00 00             	           

00010500 <_fini>:
   10500:	f1 c0                   	push_s     blink
   10502:	e0 78                   	nop_s      
   10504:	ee 0d cf ff             	bl         102f0 <__do_global_dtors_aux>

   10508:	d1 c0                   	pop_s      blink
   1050a:	e0 7e                   	j_s [blink] 

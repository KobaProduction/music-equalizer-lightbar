#!/usr/bin/env python3

# PHY62x2 Firmware Tool
# KobaProduction, 2026.
# Derived from the ROM-UART utility at https://github.com/pvvx/THB2/blob/master/rdwr_phy62x2.py
# See UPSTREAM_LICENSE.txt for upstream attribution/license.

import serial
import time
import argparse
import io
import os
import struct
import sys
import hashlib
import json
import tempfile
import urllib.request
import urllib.parse
import urllib.error
from pathlib import Path

START_BAUD = 9600
DEF_RUN_BAUD = 115200
DEV_RUN_BAUD = 500000
MAX_FLASH_SIZE = 0x200000
EXT_FLASH_ADD = 0x400000

DEF_START_RUN_APP_ADDR = 0x1FFF1838
DEF_START_WR_FLASH_ADDR = 0x05000

PHY_FLASH_SECTOR_SIZE = 4096
PHY_FLASH_SECTOR_MASK = 0xfffff000
PHY_WR_BLK_SIZE = 0x2000

PHY_FLASH_ADDR = 0x11000000
PHY_SRAM_ADDR = 0x1fff0000

__progname__ = 'PHY62x2 Firmware Tool'
__filename__ = 'phytool.py'
__version__ = "0.3.0"


def ParseHexFile(hexfile):
	try:
		fin = open(hexfile)
	except:
		print('No file opened', hexfile)
		return None
	
	table = []
	result = bytearray()
	addr = 0
	naddr = 0
	taddr = 0
	addr_flg = 0
	table.append([0, result, 0x2000])
	for hexstr in fin.readlines():
		hexstr = hexstr.strip()
		if hexstr[7:9] == '04':
			if(len(result)):
				#print(hex(addr))
				table.append([addr, result, 0])
			addr = 	int(hexstr[9:13],16) << 16
			addr_flg = 0
			result = bytearray()
			continue
		if hexstr[7:9] == '05' or hexstr[7:9] == '01':
			table.append([addr, result, 0])
			break
		taddr = (int(hexstr[3:7],16))
		if addr_flg == 0:
			addr_flg = 1
			addr = addr | taddr
			naddr = taddr
		if taddr != naddr:
			addr_flg = 1
			table.append([addr, result, 0])
			addr = (addr & 0xFFFF0000) | taddr
			result = bytearray()
		#print(hexstr[9:-3])
		result.extend(bytearray.fromhex(hexstr[9:-2]))
		naddr = taddr + int(hexstr[1:3],16)
	fin.close()
	return table

class phyflasher:
	def __init__(self, port='COM1', tm = False):
		self.tm = tm
		self.chip = 'Unknown'
		self.cpbin = 0
		self.autoerase = True
		self.old_erase_start = EXT_FLASH_ADD
		self.old_erase_end = EXT_FLASH_ADD
		self.port = port
		if tm:
			self.baud = DEF_RUN_BAUD
		else:
			self.baud = START_BAUD
		try:
			self._port = serial.Serial(self.port, self.baud)
			self._port.timeout = 1
		except Exception as e:
			print ('Error: Open %s, %d baud! Error: %s' % (self.port, self.baud, e))
			sys.exit(1)
	def SetAutoErase(self, enable = True):
		self.autoerase = enable
	def AddSectionToHead(self, addr, size):
		#self.hexf.sec[0:4] = int.to_bytes(self.hexidx, 4, byteorder='little')
		self.hexf.sec.extend(bytearray(struct.pack('<IIII', phy_head[secn][0], size, addr, 0xffffffff)))
		return self.hexf
	def write_cmd(self, pkt):
		self._port.write(pkt.encode())
		read = self._port.read(6)
		return read == b'#OK>>:'
	def SendResetCmd(self):
		return self._port.write(str.encode('reset '))
	def read_reg(self, addr):
		pkt = 'rdreg%08x' % addr
		sent = self._port.write(pkt.encode())
		read = self._port.read(17)
		if len(read) == 17 and read[0:3] == b'=0x' and read[11:17] == b'#OK>>:':
			return int(read[1:11], 16)
		return None
	def write_reg(self, addr, data):
		return self.write_cmd('wrreg%08x %08x ' % (addr, data))
	def ExpFlashSize(self):
		if self.tg7100:
			addr = 0x1fff1080
			size = self.flash_size * 2
			#return True
		else:
			addr = 0x1fff0898
			size = EXT_FLASH_ADD
		if not self.write_reg(addr, size):
			print('Error set ext.Flash size %08x!' % size)
			return False
		return True
	def wr_flash_cmd(self, cmd, data = 0, wrlen = 0, addr = 0, addrlen = 0, rdlen = 0, mbit = 0, dummy = 0):
		regcmd = cmd << 24
		if wrlen > 0:
			regcmd = regcmd | 0x8000 | ((wrlen - 1) << 12)
			if not self.write_reg(0x4000c8a8, data): #Flash Command Write Data Register
				print('Error write Flash Data Register!')
				return False
		if addrlen > 0:
			regcmd = regcmd | 0x80000 | ((addrlen - 1) << 16)
			if not self.write_reg(0x4000c894, addr): #Flash Command Write Addr Register
				print('Error write Flash Address Register!')
				return False
		if rdlen > 0:
			regcmd = regcmd | 0x800000 | ((rdlen - 1) << 20)
		if mbit > 0:
			regcmd = regcmd | 0x40000
		if dummy > 0:
			regcmd = regcmd | (dummy << 7)
		if not self.write_reg(0x4000c890, regcmd | 1):
			print('Error write Flash Command Register!')
			return False
		return True
	def flash_wait_idle(self):
		i = 5
		while i > 0:
			r = self.read_reg(0x4000c890)
			if r == None:
				return False
			if (r & 2) == 0:
				i = 5
				while i > 0:
					r = self.read_reg(0x4000c800)
					if r == None:
						return False
					if (r & 0x80000000) != 0:
						return True
					i -= 1
				return False
			i -= 1		
		return False
	def flash_read_unique_id(self):
		if self.wr_flash_cmd(0x4B,0,0,0,4,8): # and self.flash_wait_idle(): 
			r1 = self.read_reg(0x4000c8a0)
			if r1 == None:
				return None
			r2 = self.read_reg(0x4000c8a4)
			if r2 == None:
				return None
			ret = bytearray(8)
			ret[0] = r1 & 0xff 
			ret[1] = (r1 >> 8) & 0xff 
			ret[2] = (r1 >> 16) & 0xff 
			ret[3] = (r1 >> 24) & 0xff 
			ret[4] = r2 & 0xff 
			ret[5] = (r2 >> 8) & 0xff 
			ret[6] = (r2 >> 16) & 0xff 
			ret[7] = (r2 >> 24) & 0xff 
			return ret 
		return None
	def flash_read_status(self):
		#Flash cmd: Read status
		if self.wr_flash_cmd(5,0,0,0,0,1): # and self.flash_wait_idle(): 
			r = self.read_reg(0x4000c8a0)
			if r == None:
				return None
			return r & 0x0ff
		return None
	def FlashUnlock(self):
		#Flash cmd: Write Enable, Write Status Register 0x00 
		return self.wr_flash_cmd(6) and self.wr_flash_cmd(1, 0, 1)	
	def ReadRevision(self):
		#0x001364c8 6222M005 #OK>>:
		self._port.write(str.encode('rdrev+ '))
		self._port.timeout = 0.1
		read = self._port.read(26)
		#print(read)
		if len(read) == 16 and read[0:2] == b'0x' and read[10:16] == b'#OK>>:':
			print('Revision:', read[2:10])
			self.flash_id = int(read[2:10], 16)
			self.flash_size = 1 << (self.flash_id  & 0xff)
			self.chip = 'TG7100B'
			self.tg7100 = True
			print('FlashID: %06x, size: %d kbytes' % (self.flash_id, self.flash_size >> 10))
			return True
		self.tg7100 = False
		if len(read) == 26 and read[0:2] == b'0x' and read[20:26] == b'#OK>>:':
			print('Revision:', read[2:19])
			if read[11:15] == b'6230':
				self.chip = 'PHY6230'
				print('Chip PHY6230: OTP Version!')
			else:
				if read[11:15] != b'6222':
					print('Wrong Version!')
				self.chip = 'PHY6222'
				self.flash_id = int(read[2:11], 16)
				self.flash_size = 1 << ((self.flash_id >> 16) & 0xff)
				print('FlashID: %06x, size: %d kbytes' % (self.flash_id, self.flash_size >> 10))
			return True
		else:
			print('Error read Revision!')
		return False
	def SetBaud(self, baud):
		if self._port.baudrate != baud:
			print ('Reopen %s port %i baud...' % (self.port, baud), end = ' '),
			self._port.timeout = 0.7
			self._port.write(str.encode("uarts%i" % baud))
			read = self._port.read(3)
			self.baud = baud
			try:
				self._port.baudrate = baud
			except Exception as e:
				print ('Error set %i baud on %s port!' % (baud, self.port))
				sys.exit(1)
			if read != b'#OK':
				if self.read_reg(PHY_SRAM_ADDR) == None:
					print ('error!')
					print ('Error set %i baud on %s port!' % (baud, self.port))
					self._port.close()
					sys.exit(3)
			print ('ok')
			self._port.timeout = 0.2
			time.sleep(0.05)
			self._port.flushOutput()
			self._port.flushInput()
		return True
	def Connect(self, baud=DEF_RUN_BAUD):
		self._port.setRTS(True) #RSTN (lo)
		self._port.setDTR(True) #TM   (lo)
		time.sleep(0.1)
		self._port.flushOutput()
		self._port.flushInput()
		time.sleep(0.1)
		print('PHY62x2/TG7100B: Release RST_N if RTS is not connected...')
		print('ST17H66B: Turn on the power...')
		self._port.setDTR(False) #TM  (hi)
		self._port.setRTS(False) #RSTN (hi)
		self._port.timeout = 0.04
		ttcl = 250
		fct_mode = False
		pkt = 'UXTDWU' # UXTL16 UDLL48 UXTDWU
		while ttcl > 0:
			sent = self._port.write(pkt.encode())
			read = self._port.read(6)
			if read == b'cmd>>:' :
				break
			if read == b'fct>>:' :
				fct_mode = True
				break
			ttcl = ttcl - 1
			if ttcl < 1:
				print('Chip Reset error! Response: %s' % read)
				print('Check connection TX->RX, RX<-TX, RTS->RESET, TM, and Chip Power!')
				self._port.close()
				exit(4)
		print('Chip Reset Ok. Response: %s' % read)
		self._port.baudrate = DEF_RUN_BAUD
		self._port.timeout = 0.2
		if fct_mode:
			print('Chip in FCT mode!')
			return False
		if not self.ReadRevision():
			self._port.close()
			exit(4)
		if not self.FlashUnlock():
			self._port.close()
			exit(4)
		if not self.tg7100:
			if not self.write_reg(0x4000f054, 0):
				print('PHY62x2 - Error init1!')
				self._port.close()
				exit(4)
			if not self.write_reg(0x4000f140, 0):
				print('PHY62x2 - Error init2!')
				self._port.close()
				exit(4)
			if not self.write_reg(0x4000f144, 0):
				print('PHY62x2 - Error init3!')
				self._port.close()
				exit(4)
		print(self.chip, '- connected Ok')
		return self.SetBaud(baud)
	def cmd_era4k(self, offset):
		print ('Erase sector Flash at 0x%08x...' % offset, end = ' ')
		tmp = self._port.timeout
		self._port.timeout = 0.5
		ret = self.write_cmd('era4k %X' % (offset | self.flash_size))
		self._port.timeout = tmp
		if not ret:
			print ('error!')
		else:
			print ('ok')
		return ret
	def cmd_era64k(self, offset):
		print ('Erase block 64k Flash at 0x%08x...' % offset, end = ' '),
		tmp = self._port.timeout
		self._port.timeout = 2
		ret = self.write_cmd('er64k %X' % (offset | self.flash_size))
		self._port.timeout = tmp
		if not ret:
			print ('error!')
		else:
			print ('ok')
		return ret
	def cmd_er512(self, offset = 0):
		print ('Erase block 512k Flash at 0x%08x...' % offset, end = ' '),
		tmp = self._port.timeout
		self._port.timeout = 2
		if self.tg7100:
			ret = self.write_cmd('er512 ')
		else:
			ret = self.write_cmd('er512 %X' % (offset | self.flash_size))
		self._port.timeout = tmp
		if not ret:
			print ('error!')
		else:
			print ('ok')
		return ret
	def cmd_erase_work_flash(self):
		print ('Erase Flash work area...', end = ' '),
		tmp = self._port.timeout
		self._port.timeout = 7
		if self.tg7100:
			ret = self.write_cmd('etcpf ')
		else:
			ret = self.write_cmd('erall ')
		self._port.timeout = tmp
		if not ret:
			print ('error!')
		else:
			print ('ok')
		return ret
	def cmd_erase_all_flash(self):
		print ('Erase All Chip Flash...', end = ' '),
		if self.wr_flash_cmd(6) and self.wr_flash_cmd(0x60): #Write Enable, Chip Erase
			i = 77
			while i > 0:
				r = self.flash_read_status()
				if r == None:
					print ('Error!')
					return False
				if (r & 1) == 0:
					print ('ok')
					return True
				i -= 1	
		print ('Timeout!')
		return False
	def EraseSectorsFlash(self, offset = 0, size = MAX_FLASH_SIZE):
		count = int((size + PHY_FLASH_SECTOR_SIZE - 1 + (offset & (PHY_FLASH_SECTOR_SIZE - 1))) / PHY_FLASH_SECTOR_SIZE)
		offset &= PHY_FLASH_SECTOR_MASK
		if count > 0 and count < 0x10000 and offset >= 0: # 1 byte .. 16 Mbytes
			while count > 0:
				if offset >= self.old_erase_start and  offset < self.old_erase_end:
					offset += PHY_FLASH_SECTOR_SIZE
					count -= 1
					continue
				if (offset & 0x0FFFF) == 0 and count > 15:
					if not self.cmd_era64k(offset):
						return False
					self.old_erase_start = offset
					self.old_erase_end = offset + 0x10000
					offset += 0x10000
					count -= 16
				else:
					if not self.cmd_era4k(offset):
						return False
					self.old_erase_start = offset
					self.old_erase_end = offset + PHY_FLASH_SECTOR_SIZE
					offset += PHY_FLASH_SECTOR_SIZE
					count -= 1
		else:
			return False
		return True
	def send_blk(self, stream, offset, size, blkcnt, blknum, segment = 0):
		self._port.timeout = 1
		print ('Write 0x%08x bytes to Flash at 0x%08x...' % (size, offset), end = ' '),
		#if blknum == 0:  
			#if not self.write_cmd('cpnum %d ' % blkcnt):
			#	print ('error')
			#	print ('Error cmd cpnum!', read)
			#	return False
		if self.tg7100:
			self._port.write(str.encode('cpbin c%d %X %X %X' % (blknum, offset | self.flash_size, size, segment + offset)))
		else:
			self._port.write(str.encode('cpbin c%d %X %X %X' % (blknum, offset | self.flash_size, size, segment + offset)))
		read = self._port.read(12)
		if read != b'by hex mode:':
			print ('error!')
			print ('Error cmd cpbin!', read)
			return False
		data = stream.read(size)
		self._port.write(data)
		if self.tg7100:
			read = self._port.read(25)  #' checksum is: 0x00001d1e'
			if read[0:16] != b' checksum is: 0x': # TG7100
				print ('error!')
				print ('Error send bin data! ', read)
				return False
			self._port.write(read[16:24])
		else:
			read = self._port.read(23)  #'checksum is: 0x00001d1e'
			if read[0:15] != b'checksum is: 0x':
				print ('error!')
				print ('Error send bin data! ', read)
				return False
			self._port.write(read[15:])
		read = self._port.read(6)
		if read != b'#OK>>:':
			print ('error!')
			print ('Error CRC!', read)
			return False
		print ('ok')
		return True
	def WriteBlockFlash(self, stream, offset = 0, size = 0x8000, segment = PHY_SRAM_ADDR):
		offset &= 0x00ffffff
		if self.autoerase:
			if not self.EraseSectorsFlash(offset, size):	
				return False
		sblk = PHY_WR_BLK_SIZE
		blkcount = (size + sblk - 1) / sblk
		while(size > 0):
			if size < sblk:
				sblk = size
			if not self.send_blk(stream, offset, sblk, blkcount, self.cpbin, segment):
				return False
			self.cpbin+=1
			offset+=sblk
			size-=sblk
		return True
	def ReadBusToFile(self, ff, addr=PHY_FLASH_ADDR, size=0x80000):
		flg = size > 128
		if not flg:
			print('\rRead 0x%08x...' % addr, end=' ') #, flush=True
		while size > 0:
			if flg and addr & 127 == 0:
				print('\rRead 0x%08x...' % addr, end=' ') #, flush=True
			rw = self.read_reg(addr)
			if rw == None:
				print('error!')
				print('\rError read address 0x%08x!' % addr)
				return False
			dw = struct.pack('<I',rw)
			ff.write(dw)
			addr += 4
			size -= 4
		print('ok')
		return True
	def ReadAllFlash(self, ff):
		addr = PHY_FLASH_ADDR
		size = self.flash_size
		print('Read at 0x%08x, size: 0x%08x:' % (addr, size))
		while size > 0:
			if addr & 127 == 0:
				print('\rRead 0x%08x...' % addr, end=' ') #, flush=True
			rw = self.read_reg(addr)
			if rw == None:
				print('error!')
				print('\rError read address 0x%08x!' % addr)
				return None
			dw = struct.pack('<I',rw)
			ff.write(dw)
			addr += 4
			size -= 4
		print('ok')
		return self.flash_size
	def SpifsInit(self):
		if self.tm:
			return self.write_cmd('cpnum ffffffff ')
		else:
			return self.write_cmd('spifs 0 1 3 0 ') and self.write_cmd('sfmod 2 2 ') and self.write_cmd('cpnum ffffffff ')
	def HexfHeader(self, hp, start = DEF_START_RUN_APP_ADDR, raddr = DEF_START_WR_FLASH_ADDR):
		if len(hp) > 1:
			if self.tg7100:
				hp[0][2] = 0x2100
				hexf = bytearray(b'\xff')*(4)
				hexf[0:4] = int.to_bytes(len(hp)-1, 4, byteorder='little')
				#sections = 0
				faddr_min = MAX_FLASH_SIZE-1
				faddr_max = 0
				rsize = 0
				for ihp in hp:
					if (ihp[0] & PHY_SRAM_ADDR) == PHY_SRAM_ADDR:	# SRAM
						rsize += len(ihp[1])
					elif (ihp[0] & (~(MAX_FLASH_SIZE-1))) == PHY_FLASH_ADDR: # Flash
						offset = ihp[0] & (MAX_FLASH_SIZE-1)
						if faddr_min > offset:
							faddr_min = offset
						send = offset + len(ihp[1])
						if faddr_max <= send:
							faddr_max = send
				if (raddr + rsize) >= faddr_min:
					raddr = (faddr_max + 15) & 0xfffffff0
				print ('---- Segments Table -------------------------------------')
				for ihp in hp:
					if (ihp[0] & PHY_SRAM_ADDR) == PHY_SRAM_ADDR:	# SRAM
						faddr = raddr
						raddr += (len(ihp[1])+15) & 0xfffffff0
					elif (ihp[0] & (~(MAX_FLASH_SIZE-1))) == PHY_FLASH_ADDR: # Flash
						faddr = ihp[0] & (MAX_FLASH_SIZE-1)
					elif ihp[0] == 0:
						continue
					else:
						print('Invalid Segment Address 0x%08x!' % ihp[0])
						return None
					ihp[2] = faddr
					print('Segment: %08x <- Flash addr: %08x, Size: %08x' % (ihp[0], faddr, len(ihp[1])))
					hexf.extend(bytearray(struct.pack('<III', faddr, len(ihp[1]), ihp[0])))
				return hexf
			hexf = bytearray(b'\xff')*(0x100)
			hexf[0:4] = int.to_bytes(len(hp)-1, 4, byteorder='little')
			hexf[8:12] = int.to_bytes(start, 4, byteorder='little')
			#sections = 0
			faddr_min = MAX_FLASH_SIZE-1
			faddr_max = 0
			rsize = 0
			for ihp in hp:
				if (ihp[0] & PHY_SRAM_ADDR) == PHY_SRAM_ADDR:	# SRAM
					rsize += len(ihp[1])
				elif (ihp[0] & (~(MAX_FLASH_SIZE-1))) == PHY_FLASH_ADDR: # Flash
					offset = ihp[0] & (MAX_FLASH_SIZE-1)
					if faddr_min > offset:
						faddr_min = offset
					send = offset + len(ihp[1])
					if faddr_max <= send:
						faddr_max = send
			if (raddr + rsize) >= faddr_min:
				raddr = (faddr_max + 3) & 0xfffffffc
			#print('Test: Flash addr min: %08x, max: %08x, RAM addr: %08x' % (faddr_min, faddr_max, raddr))
			print ('---- Segments Table -------------------------------------')
			for ihp in hp:
				if (ihp[0] & PHY_SRAM_ADDR) == PHY_SRAM_ADDR:	# SRAM
					faddr = raddr
					raddr += (len(ihp[1])+3) & 0xfffffffc
				elif (ihp[0] & (~(MAX_FLASH_SIZE-1))) == PHY_FLASH_ADDR: # Flash
					faddr = ihp[0] & (MAX_FLASH_SIZE-1)
				elif ihp[0] == 0:
					continue
				else:
					print('Invalid Segment Address 0x%08x!' % ihp[0])
					return None
				ihp[2] = faddr
				print('Segment: %08x <- Flash addr: %08x, Size: %08x' % (ihp[0], faddr, len(ihp[1])))
				hexf.extend(bytearray(struct.pack('<IIII', faddr, len(ihp[1]), ihp[0], 0xffffffff)))
			return hexf
		return None

class FatalError(RuntimeError):
	def __init__(self, message):
		RuntimeError.__init__(self, message)

	@staticmethod
	def WithResult(message, result):
		message += " (result was %s)" % hexify(result)
		return FatalError(message)

def arg_auto_int(x):
	return int(x, 0)

def _is_url(target):
	parsed = urllib.parse.urlparse(target)
	return parsed.scheme in ("http", "https")

def _read_target_bytes(target, optional=False):
	if _is_url(target):
		req = urllib.request.Request(
			target,
			headers={"User-Agent": "phy62x2-firmware-tool/0.3"},
		)
		try:
			with urllib.request.urlopen(req, timeout=30) as response:
				return response.read()
		except urllib.error.HTTPError as exc:
			if optional and exc.code == 404:
				return None
			raise
	path = Path(target)
	if optional and not path.exists():
		return None
	return path.read_bytes()

def _manifest_candidate(target):
	if _is_url(target):
		parts = urllib.parse.urlsplit(target)
		path = parts.path
		base, ext = os.path.splitext(path)
		if ext.lower() == ".json":
			return target
		manifest_path = base + ".json"
		return urllib.parse.urlunsplit(
			(parts.scheme, parts.netloc, manifest_path, parts.query, parts.fragment)
		)
	path = Path(target)
	if path.suffix.lower() == ".json":
		return str(path)
	return str(path.with_suffix(".json"))

def _resolve_manifest_firmware(manifest_target, firmware_value):
	if _is_url(firmware_value):
		return firmware_value
	if _is_url(manifest_target):
		return urllib.parse.urljoin(manifest_target, firmware_value)
	return str((Path(manifest_target).parent / firmware_value).resolve())

def _load_manifest_target(manifest_target, optional=False):
	raw = _read_target_bytes(manifest_target, optional=optional)
	if raw is None:
		return None
	try:
		manifest = json.loads(raw.decode("utf-8"))
	except Exception as exc:
		raise FatalError("Invalid firmware manifest %s: %s" % (manifest_target, exc))
	if not isinstance(manifest, dict):
		raise FatalError("Firmware manifest must be a JSON object")
	print("Manifest:", manifest_target)
	return manifest

def prepare_flash_target(target, manifest_target=None, expected_sha256=None, auto_manifest=True):
	manifest = None
	firmware_target = target

	if target.lower().endswith(".json"):
		manifest_target = target
		manifest = _load_manifest_target(manifest_target, optional=False)
	else:
		if manifest_target:
			manifest = _load_manifest_target(manifest_target, optional=False)
		elif auto_manifest:
			candidate = _manifest_candidate(target)
			manifest = _load_manifest_target(candidate, optional=True)
			if manifest is None:
				print("Manifest: not found (%s), continuing without it" % candidate)
			else:
				manifest_target = candidate

	if manifest is not None:
		firmware_value = (
			manifest.get("firmware_url")
			or manifest.get("firmware_target")
			or manifest.get("firmware")
		)
		if target.lower().endswith(".json"):
			if not firmware_value:
				raise FatalError("Manifest does not identify firmware target")
			firmware_target = _resolve_manifest_firmware(manifest_target, firmware_value)
		elif firmware_value:
			resolved = _resolve_manifest_firmware(manifest_target, firmware_value)
			if resolved != target:
				print("Manifest firmware reference:", resolved)
				print("Explicit --target remains authoritative")

	print("Firmware target:", firmware_target)
	artifact_name = (
		os.path.basename(urllib.parse.urlparse(firmware_target).path)
		if _is_url(firmware_target)
		else Path(firmware_target).name
	)
	if artifact_name:
		print("Artifact:", urllib.parse.unquote(artifact_name))

	if manifest:
		for key, label in (
			("label", "Firmware"),
			("board", "Board"),
			("mcu", "MCU"),
			("source_commit", "Source commit"),
			("validation", "Validation"),
		):
			if manifest.get(key):
				print("%s:" % label, manifest[key])

	image = _read_target_bytes(firmware_target, optional=False)
	digest = hashlib.sha256(image).hexdigest()

	if manifest and manifest.get("size") is not None:
		if len(image) != int(manifest["size"]):
			raise FatalError(
				"Firmware size mismatch: got %d, expected %d"
				% (len(image), int(manifest["size"]))
			)

	manifest_sha = manifest.get("sha256") if manifest else None
	expected = expected_sha256 or manifest_sha
	if expected and digest.lower() != str(expected).lower():
		raise FatalError(
			"Firmware SHA-256 mismatch: got %s, expected %s"
			% (digest, expected)
		)

	print("Firmware size:", len(image), "bytes")
	print("SHA-256:", digest)
	if expected:
		print("SHA-256 verification: OK")
	else:
		print("SHA-256 verification: not requested")

	fd, path = tempfile.mkstemp(prefix="phytool-fw-", suffix=".hex")
	os.close(fd)
	with open(path, "wb") as out:
		out.write(image)

	print("Local temporary image:", path)
	print("---------------------------------------------------------")
	return manifest or {}, path


def monitor_serial_handle(ser, baud=115200):
	previous_baud = getattr(ser, "baudrate", None)
	ser.baudrate = baud
	ser.timeout = 0.02
	print(
		"UART monitor: same open port, %s -> %d baud"
		% (str(previous_baud), baud)
	)
	print("Press Ctrl+C to stop.")
	try:
		while True:
			data = ser.read(ser.in_waiting or 1)
			if data:
				sys.stdout.buffer.write(data)
				sys.stdout.buffer.flush()
	except KeyboardInterrupt:
		print("\nUART monitor stopped.")

def standalone_serial_monitor(port, baud=115200):
	print("Opening UART monitor: %s @ %d 8N1" % (port, baud))
	try:
		ser = serial.Serial(port, baudrate=baud, timeout=0.02)
	except Exception as exc:
		raise FatalError("Cannot open UART monitor on %s: %s" % (port, exc))
	try:
		monitor_serial_handle(ser, baud)
	finally:
		ser.close()

def reset_and_monitor(phy, baud=115200):
	ser = phy._port
	try:
		ser.reset_input_buffer()
	except Exception:
		try:
			ser.flushInput()
		except Exception:
			pass

	# The reset command must leave the host at the ROM baud. flush() guarantees
	# those bytes have physically left the UART before changing the host baud.
	ser.write(b"reset ")
	ser.flush()
	monitor_serial_handle(ser, baud)


def main():
	parser = argparse.ArgumentParser(
		description="%s version %s" % (__progname__, __version__),
		prog=__filename__,
	)
	parser.add_argument("--port", "-p", default="COM1", help="Serial port device")
	parser.add_argument("--tm", "-t", action="store_true", help='If pin TM is set "1"')

	subparsers = parser.add_subparsers(dest="operation", required=True)

	flash = subparsers.add_parser(
		"flash",
		help="Flash an Intel HEX from a local path or web URL",
	)
	flash.add_argument(
		"--target",
		required=True,
		help="Firmware HEX path/URL, or a JSON manifest path/URL",
	)
	flash.add_argument(
		"--manifest",
		help="Explicit manifest path/URL; otherwise <target basename>.json is probed",
	)
	flash.add_argument(
		"--no-manifest",
		action="store_true",
		help="Disable automatic sibling manifest discovery",
	)
	flash.add_argument("--sha256", help="Optional expected firmware SHA-256")
	flash.add_argument(
		"--baud",
		type=arg_auto_int,
		default=DEV_RUN_BAUD,
		help="ROM flashing baud (default: 500000)",
	)
	flash.add_argument(
		"--monitor",
		action="store_true",
		help="After reset, continue immediately on the same open serial handle",
	)
	flash.add_argument(
		"--monitor-baud",
		type=arg_auto_int,
		default=115200,
		help="Runtime UART baud (default: 115200)",
	)
	flash.add_argument(
		"--start",
		type=arg_auto_int,
		default=DEF_START_RUN_APP_ADDR,
		help="Application start address for HEX writer",
	)
	flash.add_argument(
		"--write",
		type=arg_auto_int,
		default=DEF_START_WR_FLASH_ADDR,
		help="Flash storage address for SRAM HEX sections",
	)

	dump = subparsers.add_parser("dump", help="Dump the complete external Flash")
	dump.add_argument("--target", required=True, help="Output .bin path")
	dump.add_argument(
		"--baud",
		type=arg_auto_int,
		default=DEV_RUN_BAUD,
		help="ROM transfer baud (default: 500000)",
	)

	monitor = subparsers.add_parser("monitor", help="Open runtime UART monitor")
	monitor.add_argument(
		"--baud",
		type=arg_auto_int,
		default=115200,
		help="Runtime UART baud (default: 115200)",
	)

	info = subparsers.add_parser("info", help="Read chip and Flash information")
	info.add_argument(
		"--baud",
		type=arg_auto_int,
		default=DEF_RUN_BAUD,
		help="ROM baud after connection",
	)

	args = parser.parse_args()

	if args.operation == "monitor":
		try:
			standalone_serial_monitor(args.port, args.baud)
		except Exception as exc:
			print("Error:", exc)
			sys.exit(2)
		return

	temp_path = None
	try:
		if args.operation == "flash":
			_, temp_path = prepare_flash_target(
				args.target,
				args.manifest,
				args.sha256,
				auto_manifest=not args.no_manifest,
			)
			filename = temp_path
			start_addr = args.start
			write_addr = args.write
			baud = args.baud
			do_reset = True
			do_monitor = args.monitor
			monitor_baud = args.monitor_baud
			action = "flash"
		elif args.operation == "dump":
			filename = args.target
			baud = args.baud
			action = "dump"
		elif args.operation == "info":
			baud = args.baud
			action = "info"
		else:
			raise FatalError("Unsupported operation")

		print("=========================================================")
		print("%s version %s" % (__progname__, __version__))
		print("---------------------------------------------------------")
		phy = phyflasher(args.port, args.tm)
		print("Connecting...")
		if not phy.Connect(baud):
			print("Chip is in FCT mode; normal operation is unavailable")
			sys.exit(2)

		if action == "info":
			rs = phy.flash_read_status()
			if rs is None:
				raise FatalError("Flash read status failed")
			print("Flash Status: 0x%02x" % rs)
			rb = phy.flash_read_unique_id()
			if rb is None:
				raise FatalError("Flash read unique ID failed")
			print("Flash Serial Number:", rb.hex())
			return

		if action == "dump":
			target = Path(filename)
			target.parent.mkdir(parents=True, exist_ok=True)
			with target.open("wb") as out:
				size = phy.ReadAllFlash(out)
			if size is None:
				raise FatalError("Flash dump failed")
			byte_saved = (size + 3) & 0xfffffffc
			print("---------------------------------------------------------")
			print("%d bytes saved to %s" % (byte_saved, target))
			return

		hp = ParseHexFile(filename)
		if hp is None:
			raise FatalError("Cannot parse HEX file")
		hexf = phy.HexfHeader(hp, start_addr, write_addr)
		if hexf is None:
			raise FatalError("Cannot build HEX Flash header")
		hp[0][1] = hexf

		if not phy.SpifsInit():
			raise FatalError("SPI Flash initialization failed")
		phy.SetAutoErase(True)
		if not phy.ExpFlashSize():
			raise FatalError("Flash size setup failed")

		print("----------------------------------------------------------")
		for ihp in hp:
			if ihp[0] == 0:
				print(
					"Segment Table[%02d] <- Flash addr: %08x, Size: %08x"
					% (len(hp) - 1, ihp[2], len(ihp[1]))
				)
			else:
				print(
					"Segment: %08x <- Flash addr: %08x, Size: %08x"
					% (ihp[0], ihp[2], len(ihp[1]))
				)
			stream = io.BytesIO(ihp[1])
			ok = phy.WriteBlockFlash(stream, ihp[2], len(ihp[1]), 0)
			stream.close()
			if not ok:
				raise FatalError("Flash write failed")

		print("----------------------------------------------------------")
		print("Write Flash from file: %s - ok." % filename)

		if do_reset and do_monitor:
			print("Reset -> runtime UART monitor (same serial handle)")
			reset_and_monitor(phy, monitor_baud)
		elif do_reset:
			phy.SendResetCmd()
			print("Send command 'reset' - ok")
		elif do_monitor:
			monitor_serial_handle(phy._port, monitor_baud)

	except FatalError as exc:
		print("Error:", exc)
		sys.exit(2)
	finally:
		if temp_path:
			try:
				os.unlink(temp_path)
			except OSError:
				pass

if __name__ == '__main__':
	main()

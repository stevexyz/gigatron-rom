'use strict';

/* exported setup, keyPressed, keyReleased */

var cpu;
var vga;
var blinkenLights;
var audio; // eslint-disable-line no-unused-vars
var gamepad;
var perf;

/** Performance Monitor */
class Perf {
	/** Create a Performance Monitor
	 * @param {Element} elt - The element in which to display performance
	 */
	constructor(elt) {
		this.elt = elt;
		this.cycles = 0;
		this.startTime = millis();
	}

	/** Called on start of frame */
	startOfFrame() {
		// this.cycles = 0;
	}

	/** Called at end of frame */
	refresh() {
		if (this.cycles > 5000000) {
			let endTime = millis();
			let mhz = this.cycles / (1000 * (endTime-this.startTime));
			this.elt.html(nf(mhz, 1, 3) + 'MHz');
			this.startTime = endTime;
			this.cycles = 0;
		}
		// this.elt.html(this.cycles);
		// this.cycles = 0;
	}
}

/** p5 setup function */
function setup() {
	let mhzText = createElement('h2', '--');
	perf = new Perf(mhzText);

	createCanvas(640, 480 + 44);
	noLoop();

	vga = new Vga({
		horizontal: {frontPorch: 16, backPorch: 48, visible: 640},
		vertical: {frontPorch: 10, backPorch: 34, visible: 480},
	});

  blinkenLights = new BlinkenLights();

	cpu = new Gigatron({
		log2rom: 16,
		log2ram: 15,
		onOut: (out) => vga.onOut(out),
		onOutx: (outx) => blinkenLights.onOutx(outx),
	});

	audio = new Audio(cpu);

	gamepad = new Gamepad(cpu, {
		up: UP_ARROW,
		down: DOWN_ARROW,
		left: LEFT_ARROW,
		right: RIGHT_ARROW,
		select: RETURN,
		start: ' '.codePointAt(0),
		a: 'A'.codePointAt(0),
		b: 'B'.codePointAt(0),
	});

	const romurl = 'theloop.2.rom';
	loadRom(romurl, cpu);
}

/** start the periodic */
function start() {
	setInterval(tick, 1000/60);
}

/** advance the simulation by one tick */
function tick() {
	vga.vsyncOccurred = false;

	perf.startOfFrame();

	// step simulation until next vsync (hope there is one!)
	while (!vga.vsyncOccurred) {
		cpu.tick();
		vga.tick();
		// audio.tick();
		perf.cycles++;
	}

	vga.refresh();
	perf.refresh();
}

/** KeyPressed event handler
 * @return {boolean} whether event should be default processed
*/
function keyPressed() {
return gamepad.keyPressed(keyCode);
}

/** KeyReleased event handler
 * @return {boolean} whether event should be default processed
*/
function keyReleased() {
return gamepad.keyReleased(keyCode);
}

/** async rom loader
 * @param {string} url - Url of rom file
 * @param {Gigatron} cpu - CPU to load
 */
function loadRom(url, cpu) {
	let oReq = new XMLHttpRequest();
	oReq.open('GET', url, true);
	oReq.responseType = 'arraybuffer';

	oReq.onload = function(oEvent) {
		let buffer = oReq.response;
		if (buffer) {
			let n = buffer.byteLength >> 1;
			let view = new DataView(buffer);
			for (let i = 0; i < n; i++) {
				cpu.rom[i] = view.getUint16(i<<1);
			}
			console.log('ROM loaded');
			start();
		}
	};

	oReq.send(null);
}

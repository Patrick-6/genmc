//Source: https://github.com/computersforpeace/model-checker-benchmarks/blob/master/ms-queue/main.c
//LOOP_BOUND5

#[path = "../array_queue.rs"]
mod array_queue;
use array_queue::*;

pub static mut INPUT: [u32; 2] = [0, 0];
pub static mut OUTPUT: [u32; 2] = [0, 0];

#[derive(Copy, Clone)]
struct Args<'a> {
	pid: usize,
	queue: &'a ArrayQueue<u32>
}

fn main_task(arg: Args) {
	let pid = arg.pid;
	let queue = arg.queue;
	unsafe {
		if pid == 1 {
			INPUT[0] = 17;
			queue.push(INPUT[0]).unwrap();
			match queue.pop() {
				Some(v) => {OUTPUT[0] = v;},
				None => {OUTPUT[0] = 0;}
			}
		} else {
			INPUT[1] = 37;
			queue.push(INPUT[1]).unwrap();
			match queue.pop() {
				Some(v) => {OUTPUT[1] = v;},
				None => {OUTPUT[1] = 0;}
			}
		}
	}
}


pub fn main() {
	let q = ArrayQueue::new(5);

	let h1 = std::thread::spawn_f_args(main_task, Args {pid: 0usize, queue: &q});
	let h2 = std::thread::spawn_f_args(main_task, Args {pid: 1usize, queue: &q});

	h1.join().unwrap();
	h2.join().unwrap();


	unsafe {
		let in_sum = INPUT[0] + INPUT[1];
		let out_sum = OUTPUT[0] + OUTPUT[1];
		assert!(in_sum == out_sum);
	}

	drop(q)
}
#ifndef __COGUTIL_SIGSLOT_H__
#define __COGUTIL_SIGSLOT_H__

#include <functional>
#include <map>
#include <mutex>

// A signal-slot class, to replace boost::signals2, which is painfully
// over-weight, complex and slow. (Try it -- launch gdb, get into the
// signal, and look at the stack. boost::signals2 uses eleven stack
// frames to do its thing. Eleven! Really!) So this class avoids that.
//
// We don't need anything fancy; this is super-minimalist in design.
// It is thread-safe. It works!
//
// Example usage:
//
// void foo(int x, std::vector<int> y) {
//    printf ("ola %d (%d) [%d %d %d]\n", x, y.size(), y[0], y[1], y[2]);
// }
//
// main () {
//     SigSlot<int, std::vector<int>> siggy;
//     siggy.connect(foo);
//     siggy.emit(42, {68,69,70});
// }
//
// The following also works:
//
// class Bar { public:
//     void baz(int x, std::vector<int> y) {
//         printf ("ciao %d\n", x);
//     }};
//
// main () {
//     Bar bell;
//     SigSlot<int, std::vector<int>> siggy;
//     auto glub = std::bind(&Bar::baz, bell,
//              std::placeholders::_1, std::placeholders::_2);
//     siggy.connect(glub);
//     siggy.emit(42, {68,69,70});
// }

template <typename... ARGS>
class SigSlot
{
	public:
		typedef std::function<void(ARGS...)> slot_type;

	private:
		mutable std::mutex _mtx;
		mutable std::map<int, std::function<void(ARGS...)>> _slots;
		mutable int _slot_id;

	public:
		SigSlot() : _slot_id(0) {}

		// Connect using std::function.
		int connect(std::function<void(ARGS...)> const& fn)
		{
			std::lock_guard<std::mutex> lck(_mtx);
			_slot_id++;
			_slots.insert(std::make_pair(_slot_id, fn));
			return _slot_id;
		}

		// Connect member of a given object.
		//
		// class Bar { public:
		//     void baz(int x, std::vector<int> y) {
		//         printf ("ciao %d\n", x);
		//     }
		// };
		//
		// main() {
		//     Bar bell;
		//     SigSlot<int, std::vector<int>> siggy;
		//     siggy.connect(&Bar::baz, bell);
		//     siggy.emit(42, {68,69,70});
		// }
		template <typename Class>
		int connect(void (Class::*fn)(ARGS...), Class* obj)
		{
			return connect([obj, fn](ARGS... args) { (obj->*fn)(args...); });
		}

		void disconnect(int slotid)
		{
			std::lock_guard<std::mutex> lck(_mtx);
			auto it = _slots.find(slotid);
			if (it != _slots.end())
				_slots.erase(it);
		}

		void disconnect_all()
		{
			std::lock_guard<std::mutex> lck(_mtx);
			_slots.clear();
		}

		// Call everything that's connected.
		void emit(ARGS... p)
		{
			// Avoid taking the lock if there's nothing to deliver.
			if (0 == _slots.size()) return;
			std::lock_guard<std::mutex> lck(_mtx);
			for (auto it : _slots) it.second(p...);
		}

		// Do not take a lock. We want this to be fast.
		size_t size() { return _slots.size(); }
};

#endif /* __COGUTIL_SIGSLOT_H__ */

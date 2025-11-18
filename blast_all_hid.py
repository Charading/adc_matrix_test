import hid
import time

PAYLOAD = bytes([0x00, 0x10, 0x01, 0x02] + [0x00] * 28)

def blast_all_hid():
    devices = hid.enumerate()

    print("Found", len(devices), "HID interfaces\n")

    for i, d in enumerate(devices):
        print("=" * 80)
        print(f"[{i}] Testing interface:")
        print(f"  vendor_id:      {hex(d['vendor_id'])}")
        print(f"  product_id:     {hex(d['product_id'])}")
        print(f"  usage_page:     {d.get('usage_page')}")
        print(f"  usage:          {d.get('usage')}")
        print(f"  interface_num:  {d.get('interface_number')}")
        print(f"  path:           {d['path']}")
        print()

        try:
            dev = hid.device()
            dev.open_path(d['path'])

            print(f"--> Sending payload to path index {i}")
            written = dev.write(PAYLOAD)
            print("    bytes written:", written)

            time.sleep(0.05)

            try:
                r = dev.read(32, timeout_ms=20)
                print("    ACK read:", r)
            except Exception as e:
                print("    No ACK:", e)

            dev.close()

        except Exception as e:
            print(f"    FAILED to open/write: {e}")

    print("\nDone.")
    print("=" * 80)


if __name__ == "__main__":
    blast_all_hid()

# Audio To MIDI (VST3 / AU)

ປລັກອິນແປງສຽງຮ້ອງ/ທຳນອງສຽງດຽວ ເປັນ MIDI ແບບ real-time, ອັດໄດ້, ແລະບັນທຶກ/ລາກເປັນໄຟລ໌ .mid

## ວິທີໄດ້ໄຟລ໌ຕິດຕັ້ງ (ບໍ່ຕ້ອງລົງເຄື່ອງມື)
1. ສ້າງ repo ໃໝ່ເທິງ github.com ແລ້ວອັບໂຫຼດໂຟນເດີນີ້ທັງໝົດ (ລວມ `.github`)
2. ເປີດແຖບ **Actions** ລໍຖ້າ build ປະມານ 10-15 ນາທີ
3. ເຂົ້າ run ທີ່ສຳເລັດ ແລ້ວໂຫຼດ **AudioToMidi-Windows** ຫຼື **AudioToMidi-macOS**

## ຕິດຕັ້ງ
- **Windows**: ກ໊ອບ `Audio To MIDI.vst3` ໄປ `C:\Program Files\Common Files\VST3\`
- **macOS**: ແຕກ tar.gz, ກ໊ອບ `.vst3` ໄປ `~/Library/Audio/Plug-Ins/VST3/` ແລະ `.component` ໄປ `~/Library/Audio/Plug-Ins/Components/`
  ຖ້າ macOS ບລັອກ: `xattr -dr com.apple.quarantine "<ໄຟລ໌>"`
- ແລ້ວ rescan plugin ໃນ DAW

## ວິທີໃຊ້
ໃສ່ plugin ເທິງ track ສຽງຮ້ອງ → ຕັ້ງ MIDI output ໄປ instrument → ກົດ Record ຮ້ອງ → Save .mid ຫຼື ລາກກ່ອງສີສົ້ມໄປວາງໃນ track.
ບາງ DAW (ເຊັ່ນ Ableton) ບໍ່ສົ່ງ MIDI ອອກຈາກ plugin ໂດຍກົງ — ໃຊ້ Save/ລາກໄຟລ໌ .mid ແທນ.

## Build ເອງ (ທາງເລືອກ)
    cmake -B build -DCMAKE_BUILD_TYPE=Release -DAM_INSTALL=ON
    cmake --build build --config Release

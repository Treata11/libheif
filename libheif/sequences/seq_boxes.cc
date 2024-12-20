/*
 * HEIF codec.
 * Copyright (c) 2024 Dirk Farin <dirk.farin@gmail.com>
 *
 * This file is part of libheif.
 *
 * libheif is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * libheif is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libheif.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "sequences/seq_boxes.h"


Error Box_container::parse(BitstreamRange& range, const heif_security_limits* limits)
{
  return read_children(range, READ_CHILDREN_ALL, limits);
}


std::string Box_container::dump(Indent& indent) const
{
  std::ostringstream sstr;
  sstr << Box::dump(indent);
  sstr << dump_children(indent);

  return sstr.str();
}


double Box_mvhd::get_matrix_element(int idx) const
{
  if (idx == 8) {
    return 1.0;
  }

  return m_matrix[idx] / double(0x10000);
}


Error Box_mvhd::parse(BitstreamRange& range, const heif_security_limits* limits)
{
  parse_full_box_header(range);

  if (get_version() > 1) {
    return unsupported_version_error("mvhd");
  }

  if (get_version() == 1) {
    m_creation_time = range.read64();
    m_modification_time = range.read64();
    m_timescale = range.read64();
    m_duration = range.read64();
  }
  else {
    // version==0
    m_creation_time = range.read32();
    m_modification_time = range.read32();
    m_timescale = range.read32();
    m_duration = range.read32();
  }

  m_rate = range.read32();
  m_volume = range.read16();
  range.skip(2);
  range.skip(8);
  for (int i = 0; i < 9; i++) {
    m_matrix[i] = range.read32();
  }
  for (int i = 0; i < 6; i++) {
    range.skip(4);
  }

  m_next_track_ID = range.read32();

  return range.get_error();
}


std::string Box_mvhd::dump(Indent& indent) const
{
  std::ostringstream sstr;
  sstr << Box::dump(indent);
  sstr << indent << "creation time:     " << m_creation_time << "\n"
      << indent << "modification time: " << m_modification_time << "\n"
      << indent << "timescale: " << m_timescale << "\n"
      << indent << "duration: " << m_duration << "\n";
  sstr << indent << "rate: " << get_rate() << "\n"
      << indent << "volume: " << get_volume() << "\n"
      << indent << "matrix:\n";
  for (int y = 0; y < 3; y++) {
    sstr << indent << "  ";
    for (int i = 0; i < 3; i++) {
      sstr << get_matrix_element(i + 3 * y) << " ";
    }
    sstr << "\n";
  }
  sstr << indent << "next_track_ID: " << m_next_track_ID << "\n";

  return sstr.str();
}


void Box_mvhd::derive_box_version()
{
  if (m_creation_time > 0xFFFFFFFF ||
      m_modification_time > 0xFFFFFFFF ||
      m_timescale > 0xFFFFFFFF ||
      m_duration > 0xFFFFFFFF) {
    set_version(1);
  }
  else {
    set_version(0);
  }
}


Error Box_mvhd::write(StreamWriter& writer) const
{
  size_t box_start = reserve_box_header_space(writer);

  if (get_version() == 1) {
    writer.write64(m_creation_time);
    writer.write64(m_modification_time);
    writer.write64(m_timescale);
    writer.write64(m_duration);
  }
  else {
    // version==0
    writer.write32(static_cast<uint32_t>(m_creation_time));
    writer.write32(static_cast<uint32_t>(m_modification_time));
    writer.write32(static_cast<uint32_t>(m_timescale));
    writer.write32(static_cast<uint32_t>(m_duration));
  }

  writer.write32(m_rate);
  writer.write16(m_volume);
  writer.write16(0);
  writer.write64(0);
  for (int i = 0; i < 9; i++) {
    writer.write32(m_matrix[i]);
  }
  for (int i = 0; i < 6; i++) {
    writer.write32(0);
  }

  writer.write32(m_next_track_ID);

  prepend_header(writer, box_start);

  return Error::Ok;
}


double Box_tkhd::get_matrix_element(int idx) const
{
  if (idx == 8) {
    return 1.0;
  }

  return m_matrix[idx] / double(0x10000);
}


Error Box_tkhd::parse(BitstreamRange& range, const heif_security_limits* limits)
{
  parse_full_box_header(range);

  if (get_version() > 1) {
    return unsupported_version_error("tkhd");
  }

  if (get_version() == 1) {
    m_creation_time = range.read64();
    m_modification_time = range.read64();
    m_track_id = range.read32();
    range.skip(4);
    m_duration = range.read64();
  }
  else {
    // version==0
    m_creation_time = range.read32();
    m_modification_time = range.read32();
    m_track_id = range.read32();
    range.skip(4);
    m_duration = range.read32();
  }

  range.skip(8);
  m_layer = range.read16();
  m_alternate_group = range.read16();
  m_volume = range.read16();
  range.skip(2);
  for (int i = 0; i < 9; i++) {
    m_matrix[i] = range.read32();
  }

  m_width = range.read32();
  m_height = range.read32();

  return range.get_error();
}


std::string Box_tkhd::dump(Indent& indent) const
{
  std::ostringstream sstr;
  sstr << Box::dump(indent);
  sstr << indent << "creation time:     " << m_creation_time << "\n"
      << indent << "modification time: " << m_modification_time << "\n"
      << indent << "track ID: " << m_track_id << "\n"
      << indent << "duration: " << m_duration << "\n";
  sstr << indent << "layer: " << m_layer << "\n"
      << indent << "alternate_group: " << m_alternate_group << "\n"
      << indent << "volume: " << get_volume() << "\n"
      << indent << "matrix:\n";
  for (int y = 0; y < 3; y++) {
    sstr << indent << "  ";
    for (int i = 0; i < 3; i++) {
      sstr << get_matrix_element(i + 3 * y) << " ";
    }
    sstr << "\n";
  }

  sstr << indent << "width: " << get_width() << "\n"
      << indent << "height: " << get_height() << "\n";

  return sstr.str();
}


void Box_tkhd::derive_box_version()
{
  if (m_creation_time > 0xFFFFFFFF ||
      m_modification_time > 0xFFFFFFFF ||
      m_duration > 0xFFFFFFFF) {
    set_version(1);
  }
  else {
    set_version(0);
  }
}


Error Box_tkhd::write(StreamWriter& writer) const
{
  size_t box_start = reserve_box_header_space(writer);

  if (get_version() == 1) {
    writer.write64(m_creation_time);
    writer.write64(m_modification_time);
    writer.write32(m_track_id);
    writer.write32(0);
    writer.write64(m_duration);
  }
  else {
    // version==0
    writer.write32(static_cast<uint32_t>(m_creation_time));
    writer.write32(static_cast<uint32_t>(m_modification_time));
    writer.write32(m_track_id);
    writer.write32(0);
    writer.write32(static_cast<uint32_t>(m_duration));
  }

  writer.write64(0);
  writer.write16(m_layer);
  writer.write16(m_alternate_group);
  writer.write16(m_volume);
  writer.write16(0);
  for (int i = 0; i < 9; i++) {
    writer.write32(m_matrix[i]);
  }

  writer.write32(m_width);
  writer.write32(m_height);

  prepend_header(writer, box_start);

  return Error::Ok;
}


Error Box_mdhd::parse(BitstreamRange& range, const heif_security_limits* limits)
{
  parse_full_box_header(range);

  if (get_version() > 1) {
    return unsupported_version_error("mdhd");
  }

  if (get_version() == 1) {
    m_creation_time = range.read64();
    m_modification_time = range.read64();
    m_timescale = range.read32();
    m_duration = range.read64();
  }
  else {
    // version==0
    m_creation_time = range.read32();
    m_modification_time = range.read32();
    m_timescale = range.read32();
    m_duration = range.read32();
  }

  uint16_t language_packed = range.read16();
  m_language[0] = ((language_packed >> 10) & 0x1F) + 0x60;
  m_language[1] = ((language_packed >> 5) & 0x1F) + 0x60;
  m_language[2] = ((language_packed >> 0) & 0x1F) + 0x60;
  m_language[3] = 0;

  range.skip(2);

  return range.get_error();
}


std::string Box_mdhd::dump(Indent& indent) const
{
  std::ostringstream sstr;
  sstr << Box::dump(indent);
  sstr << indent << "creation time:     " << m_creation_time << "\n"
      << indent << "modification time: " << m_modification_time << "\n"
      << indent << "timescale: " << m_timescale << "\n"
      << indent << "duration: " << m_duration << "\n";
  sstr << indent << "language: " << m_language << "\n";

  return sstr.str();
}


void Box_mdhd::derive_box_version()
{
  if (m_creation_time > 0xFFFFFFFF ||
      m_modification_time > 0xFFFFFFFF ||
      m_duration > 0xFFFFFFFF) {
    set_version(1);
  }
  else {
    set_version(0);
  }
}


Error Box_mdhd::write(StreamWriter& writer) const
{
  size_t box_start = reserve_box_header_space(writer);

  if (get_version() == 1) {
    writer.write64(m_creation_time);
    writer.write64(m_modification_time);
    writer.write32(m_timescale);
    writer.write64(m_duration);
  }
  else {
    // version==0
    writer.write32(static_cast<uint32_t>(m_creation_time));
    writer.write32(static_cast<uint32_t>(m_modification_time));
    writer.write32(m_timescale);
    writer.write32(static_cast<uint32_t>(m_duration));
  }

  uint16_t language_packed = ((((m_language[0] - 0x60) & 0x1F) << 10) |
                              (((m_language[1] - 0x60) & 0x1F) << 5) |
                              (((m_language[2] - 0x60) & 0x1F) << 0));
  writer.write16(language_packed);
  writer.write16(0);

  prepend_header(writer, box_start);

  return Error::Ok;
}



Error Box_vmhd::parse(BitstreamRange& range, const heif_security_limits* limits)
{
  parse_full_box_header(range);

  if (get_version() > 0) {
    return unsupported_version_error("vmhd");
  }

  m_graphics_mode = range.read16();
  for (int i = 0; i < 3; i++) {
    m_op_color[i] = range.read16();
  }

  return range.get_error();
}


std::string Box_vmhd::dump(Indent& indent) const
{
  std::ostringstream sstr;
  sstr << Box::dump(indent);
  sstr << indent << "graphics mode: " << m_graphics_mode;
  if (m_graphics_mode == 0) {
    sstr << " (copy)";
  }
  sstr << "\n"
       << indent << "op color: " << m_op_color[0] << "; " << m_op_color[1] << "; " << m_op_color[2] << "\n";

  return sstr.str();
}


Error Box_vmhd::write(StreamWriter& writer) const
{
  size_t box_start = reserve_box_header_space(writer);

  writer.write16(m_graphics_mode);
  for (int i = 0; i < 3; i++) {
    writer.write16(m_op_color[i]);
  }

  prepend_header(writer, box_start);

  return Error::Ok;
}


Error Box_stsd::parse(BitstreamRange& range, const heif_security_limits* limits)
{
  parse_full_box_header(range);

  if (get_version() > 0) {
    return unsupported_version_error("stsd");
  }

  uint32_t entry_count = range.read32();
  for (uint32_t i = 0; i < entry_count; i++) {
    std::shared_ptr<Box> entrybox;
    Error err = Box::read(range, &entrybox, limits);
    if (err) {
      return err;
    }

    auto visualSampleEntry_box = std::dynamic_pointer_cast<Box_VisualSampleEntry>(entrybox);
    if (!visualSampleEntry_box) {
      return Error{heif_error_Invalid_input,
                   heif_suberror_Unspecified,
                   "Invalid or unknown VisualSampleEntry in stsd box."};
    }

    m_sample_entries.push_back(visualSampleEntry_box);
  }

  return range.get_error();
}


std::string Box_stsd::dump(Indent& indent) const
{
  std::ostringstream sstr;
  sstr << Box::dump(indent);
  for (size_t i = 0; i < m_sample_entries.size(); i++) {
    sstr << indent << "[" << i << "]\n";
    indent++;
    sstr << m_sample_entries[i]->dump(indent);
    indent--;
  }

  return sstr.str();
}


Error Box_stsd::write(StreamWriter& writer) const
{
  size_t box_start = reserve_box_header_space(writer);

  writer.write32(static_cast<uint32_t>(m_sample_entries.size()));
  for (const auto& sample : m_sample_entries) {
    sample->write(writer);
  }

  prepend_header(writer, box_start);

  return Error::Ok;
}


Error Box_stts::parse(BitstreamRange& range, const heif_security_limits* limits)
{
  parse_full_box_header(range);

  if (get_version() > 0) {
    return unsupported_version_error("stts");
  }

  uint32_t entry_count = range.read32();
  for (uint32_t i = 0; i < entry_count; i++) {
    TimeToSample entry;
    entry.sample_count = range.read32();
    entry.sample_delta = range.read32();
    m_entries.push_back(entry);
  }

  return range.get_error();
}


std::string Box_stts::dump(Indent& indent) const
{
  std::ostringstream sstr;
  sstr << Box::dump(indent);
  for (size_t i = 0; i < m_entries.size(); i++) {
    sstr << indent << "[" << i << "] : cnt=" << m_entries[i].sample_count << ", delta=" << m_entries[i].sample_delta << "\n";
  }

  return sstr.str();
}


Error Box_stts::write(StreamWriter& writer) const
{
  size_t box_start = reserve_box_header_space(writer);

  writer.write32(static_cast<uint32_t>(m_entries.size()));
  for (const auto& sample : m_entries) {
    writer.write32(sample.sample_count);
    writer.write32(sample.sample_delta);
  }

  prepend_header(writer, box_start);

  return Error::Ok;
}


uint32_t Box_stts::get_sample_duration(uint32_t sample_idx)
{
  size_t i = 0;
  while (i < m_entries.size()) {
    if (sample_idx < m_entries[i].sample_count) {
      return m_entries[i].sample_delta;
    }
    else {
      sample_idx -= m_entries[i].sample_count;
    }
  }

  return 0;
}


void Box_stts::append_sample_duration(uint32_t duration)
{
  if (m_entries.empty() || m_entries.back().sample_delta != duration) {
    TimeToSample entry;
    entry.sample_delta = duration;
    entry.sample_count = 1;
    m_entries.push_back(entry);
    return;
  }

  m_entries.back().sample_count++;
}


uint64_t Box_stts::get_total_duration(bool include_last_frame_duration)
{
  uint64_t total = 0;

  for (const auto& entry : m_entries) {
    total += entry.sample_count * uint64_t(entry.sample_delta);
  }

  if (!include_last_frame_duration && !m_entries.empty()) {
    total -= m_entries.back().sample_delta;
  }

  return total;
}


Error Box_stsc::parse(BitstreamRange& range, const heif_security_limits* limits)
{
  parse_full_box_header(range);

  if (get_version() > 0) {
    return unsupported_version_error("stsc");
  }

  uint32_t entry_count = range.read32();
  for (uint32_t i = 0; i < entry_count; i++) {
    SampleToChunk entry;
    entry.first_chunk = range.read32();
    entry.samples_per_chunk = range.read32();
    entry.sample_description_index = range.read32();
    m_entries.push_back(entry);
  }

  return range.get_error();
}


std::string Box_stsc::dump(Indent& indent) const
{
  std::ostringstream sstr;
  sstr << Box::dump(indent);
  for (size_t i = 0; i < m_entries.size(); i++) {
    sstr << indent << "[" << i << "]\n"
        << indent << "  first chunk: " << m_entries[i].first_chunk << "\n"
        << indent << "  samples per chunk: " << m_entries[i].samples_per_chunk << "\n"
        << indent << "  sample description index: " << m_entries[i].sample_description_index << "\n";
  }

  return sstr.str();
}


Error Box_stsc::write(StreamWriter& writer) const
{
  size_t box_start = reserve_box_header_space(writer);

  writer.write32(static_cast<uint32_t>(m_entries.size()));
  for (const auto& sample : m_entries) {
    writer.write32(sample.first_chunk);
    writer.write32(sample.samples_per_chunk);
    writer.write32(sample.sample_description_index);
  }

  prepend_header(writer, box_start);

  return Error::Ok;
}


const Box_stsc::SampleToChunk* Box_stsc::get_chunk(uint32_t idx) const
{
  assert(idx>=1);
  for (size_t i = 0 ; i < m_entries.size();i++) {
    if (idx >= m_entries[i].first_chunk && (i==m_entries.size()-1 || idx < m_entries[i+1].first_chunk)) {
      return &m_entries[i];
    }
  }

  return nullptr;
}


void Box_stsc::add_chunk(uint32_t description_index)
{
  SampleToChunk entry;
  entry.first_chunk = 1; // TODO
  entry.samples_per_chunk = 0;
  entry.sample_description_index = description_index;
  m_entries.push_back(entry);
}


void Box_stsc::increase_samples_in_chunk(uint32_t nFrames)
{
  assert(!m_entries.empty());

  m_entries.back().samples_per_chunk += nFrames;
}


Error Box_stco::parse(BitstreamRange& range, const heif_security_limits* limits)
{
  parse_full_box_header(range);

  if (get_version() > 0) {
    return unsupported_version_error("stco");
  }

  uint32_t entry_count = range.read32();
  for (uint32_t i = 0; i < entry_count; i++) {
    m_offsets.push_back(range.read32());
  }

  return range.get_error();
}


std::string Box_stco::dump(Indent& indent) const
{
  std::ostringstream sstr;
  sstr << Box::dump(indent);
  for (size_t i = 0; i < m_offsets.size(); i++) {
    sstr << indent << "[" << i << "] : 0x" << std::hex << m_offsets[i] << std::dec << "\n";
  }

  return sstr.str();
}


Error Box_stco::write(StreamWriter& writer) const
{
  size_t box_start = reserve_box_header_space(writer);

  writer.write32(static_cast<uint32_t>(m_offsets.size()));

  m_offset_start_pos = writer.get_position();

  for (uint32_t offset : m_offsets) {
    writer.write32(offset);
  }

  prepend_header(writer, box_start);

  return Error::Ok;
}


void Box_stco::patch_file_pointers(StreamWriter& writer, size_t offset)
{
  size_t oldPosition = writer.get_position();

  writer.set_position(m_offset_start_pos);

  for (uint32_t chunk_offset : m_offsets) {
    if (chunk_offset + offset > std::numeric_limits<uint32_t>::max()) {
      writer.write32(0); // TODO: error
    }
    else {
      writer.write32(static_cast<uint32_t>(chunk_offset + offset));
    }
  }

  writer.set_position(oldPosition);
}



Error Box_stsz::parse(BitstreamRange& range, const heif_security_limits* limits)
{
  parse_full_box_header(range);

  if (get_version() > 0) {
    return unsupported_version_error("stsz");
  }

  m_fixed_sample_size = range.read32();
  m_sample_count = range.read32();

  if (m_fixed_sample_size == 0) {
    for (uint32_t i = 0; i < m_sample_count; i++) {
      m_sample_sizes.push_back(range.read32());
    }
  }

  return range.get_error();
}


std::string Box_stsz::dump(Indent& indent) const
{
  std::ostringstream sstr;
  sstr << Box::dump(indent);
  sstr << indent << "sample count: " << m_sample_count << "\n";
  if (m_fixed_sample_size == 0) {
    for (size_t i = 0; i < m_sample_sizes.size(); i++) {
      sstr << indent << "[" << i << "] : " << m_sample_sizes[i] << "\n";
    }
  }
  else {
    sstr << indent << "fixed sample size: " << m_fixed_sample_size << "\n";
  }

  return sstr.str();
}


Error Box_stsz::write(StreamWriter& writer) const
{
  size_t box_start = reserve_box_header_space(writer);

  writer.write32(m_fixed_sample_size);
  writer.write32(m_sample_count);
  if (m_fixed_sample_size == 0) {
    assert(m_sample_count == m_sample_sizes.size());

    for (uint32_t size : m_sample_sizes) {
      writer.write32(size);
    }
  }

  prepend_header(writer, box_start);

  return Error::Ok;
}


void Box_stsz::append_sample_size(uint32_t size)
{
  if (m_sample_count == 0) {
    m_fixed_sample_size = size;
    m_sample_count = 1;
    return;
  }

  if (m_fixed_sample_size == size) {
    m_sample_count++;
    return;
  }

  if (m_fixed_sample_size != 0) {
    for (uint32_t i = 0; i < m_sample_count; i++) {
      m_sample_sizes.push_back(m_fixed_sample_size);
    }

    m_fixed_sample_size = 0;
  }

  m_sample_sizes.push_back(size);
  m_sample_count++;

  assert(m_sample_count == m_sample_sizes.size());
}


Error Box_stss::parse(BitstreamRange& range, const heif_security_limits* limits)
{
  parse_full_box_header(range);

  if (get_version() > 0) {
    return unsupported_version_error("stss");
  }

  uint32_t sample_count = range.read32();

  for (uint32_t i = 0; i < sample_count; i++) {
    m_sync_samples.push_back(range.read32());
  }

  return range.get_error();
}


std::string Box_stss::dump(Indent& indent) const
{
  std::ostringstream sstr;
  sstr << Box::dump(indent);
  for (size_t i = 0; i < m_sync_samples.size(); i++) {
    sstr << indent << "[" << i << "] : " << m_sync_samples[i] << "\n";
  }

  return sstr.str();
}


Error Box_stss::write(StreamWriter& writer) const
{
  size_t box_start = reserve_box_header_space(writer);

  writer.write32(static_cast<uint32_t>(m_sync_samples.size()));
  for (uint32_t sample : m_sync_samples) {
    writer.write32(sample);
  }

  prepend_header(writer, box_start);

  return Error::Ok;
}


Error VisualSampleEntry::parse(BitstreamRange& range, const heif_security_limits* limits)
{
  range.skip(6);
  data_reference_index = range.read16();

  pre_defined = range.read16();
  range.skip(2);
  for (int i=0;i<3;i++) {
    pre_defined2[i] = range.read32();
  }
  width = range.read16();
  height = range.read16();
  horizresolution = range.read32();
  vertresolution = range.read32();
  range.skip(4);
  frame_count = range.read16();
  compressorname = range.read_fixed_string(32);
  depth = range.read16();
  pre_defined3 = range.read16();

  // other boxes from derived specifications
  //std::shared_ptr<Box_clap> clap; // optional // TODO
  //std::shared_ptr<Box_pixi> pixi; // optional // TODO

  return Error::Ok;
}


Error VisualSampleEntry::write(StreamWriter& writer) const
{
  writer.write32(0);
  writer.write16(0);
  writer.write16(data_reference_index);

  writer.write16(pre_defined);
  writer.write16(0);
  for (int i=0;i<3;i++) {
    writer.write32(pre_defined2[i]);
  }

  writer.write16(width);
  writer.write16(height);
  writer.write32(horizresolution);
  writer.write32(vertresolution);
  writer.write32(0);
  writer.write16(frame_count);
  writer.write_fixed_string(compressorname, 32);
  writer.write16(depth);
  writer.write16(pre_defined3);

  return Error::Ok;
}


std::string VisualSampleEntry::dump(Indent& indent) const
{
  std::stringstream sstr;
  sstr << indent << "data reference index: " << data_reference_index << "\n"
       << indent << "width: " << width << "\n"
       << indent << "height: " << height << "\n"
       << indent << "horiz. resolution: " << get_horizontal_resolution() << "\n"
       << indent << "vert. resolution: " << get_vertical_resolution() << "\n"
       << indent << "frame count: " << frame_count << "\n"
       << indent << "compressorname: " << compressorname << "\n"
       << indent << "depth: " << depth << "\n";

  return sstr.str();
}


Error Box_ccst::parse(BitstreamRange& range, const heif_security_limits* limits)
{
  parse_full_box_header(range);

  if (get_version() > 0) {
    return unsupported_version_error("ccst");
  }

  uint32_t bits = range.read32();

  auto& constraints = m_codingConstraints;

  constraints.all_ref_pics_intra = (bits & 0x80000000) != 0;
  constraints.intra_pred_used = (bits & 0x40000000) != 0;
  constraints.max_ref_per_pic = (bits >> 26) & 0x0F;

  return range.get_error();
}


std::string Box_ccst::dump(Indent& indent) const
{
  const auto& constraints = m_codingConstraints;

  std::ostringstream sstr;
  sstr << Box::dump(indent);
  sstr << indent << "all ref pics intra: " << std::boolalpha <<constraints.all_ref_pics_intra << "\n"
       << indent << "intra pred used: " << constraints.intra_pred_used << "\n"
       << indent << "max ref per pic: " << ((int) constraints.max_ref_per_pic) << "\n";

  return sstr.str();
}


Error Box_ccst::write(StreamWriter& writer) const
{
  const auto& constraints = m_codingConstraints;

  size_t box_start = reserve_box_header_space(writer);

  uint32_t bits = 0;

  if (constraints.all_ref_pics_intra) {
    bits |= 0x80000000;
  }

  if (constraints.intra_pred_used) {
    bits |= 0x40000000;
  }

  bits |= constraints.max_ref_per_pic << 26;

  writer.write32(bits);

  prepend_header(writer, box_start);

  return Error::Ok;
}


Error Box_VisualSampleEntry::write(StreamWriter& writer) const
{
  size_t box_start = reserve_box_header_space(writer);

  Error err = get_VisualSampleEntry_const().write(writer);
  if (err) {
    return err;
  }

  write_children(writer);

  prepend_header(writer, box_start);

  return Error::Ok;
}


std::string Box_VisualSampleEntry::dump(Indent& indent) const
{
  std::stringstream sstr;
  sstr << Box::dump(indent);
  sstr << m_visualSampleEntry.dump(indent);
  sstr << dump_children(indent);
  return sstr.str();
}


Error Box_VisualSampleEntry::parse(BitstreamRange& range, const heif_security_limits* limits)
{
  auto err = m_visualSampleEntry.parse(range, limits);
  if (err) {
    return err;
  }

  err = read_children(range, READ_CHILDREN_ALL, limits);
  if (err) {
    return err;
  }

  return Error::Ok;
}

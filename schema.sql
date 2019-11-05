-- PostgreSQL schema for picam recorder


CREATE TABLE sensors (
    id serial PRIMARY KEY,
    mac_address macaddr UNIQUE NOT NULL,
    serial_number char(16) UNIQUE NOT NULL
);

--CREATE TABLE sensor_positions (
--    id integer PRIMARY KEY,
--    sensor_id integer REFERENCES sensors (id),
--    latitude real,
--    longitude real,
--);

CREATE TABLE images (
    id serial PRIMARY KEY,
    image bytea
);

CREATE TABLE image_metadata (
    id serial PRIMARY KEY,
    image_id integer REFERENCES images (id),
    sensor_id integer REFERENCES sensors (id),
    time timestamp with time zone,
    latitude real,
    longitude real,
    altitude real,
    width integer,
    height integer,
    encoding char(8),
    analog_gain real,
    awb_gain_red real,
    awb_gain_blue real,
    awb_mode char(16),
    brightness integer,
    contrast integer,
    digital_gain real,
    drc_strength char(8),
    exposure_compensation integer,
    exposure_mode char(16),
    exposure_speed integer,
    hflip boolean,
    image_denoise boolean,
    --image_effect string,
    iso integer,
    --meter_mode string,
    rotation integer,
    saturation integer,
    sharpness integer,
    shutter_speed integer,
    vflip boolean,
    --video_denoise boolean,
    --video_stabilization boolean,
    roi box -- AKA zoom
);

-- PostgreSQL schema for picam recorder


CREATE TABLE sensors (
    id integer PRIMARY KEY,
    mac_address macaddr UNIQUE NOT NULL,
    serial_number string UNIQUE NOT NULL,
);

--CREATE TABLE sensor_positions (
--    id integer PRIMARY KEY,
--    sensor_id integer REFERENCES sensors (id),
--    latitude real,
--    longitude real,
--);

CREATE TABLE images (
    id integer PRIMARY KEY,
    image bytea,
);

CREATE TABLE image_metadata (
    id integer PRIMARY KEY,
    image_id integer REFERENCES images (id),
    sensor_id integer REFERENCES sensors (id),
    time timestamp with time zone,
    latitude real,
    longitude real,
    altitude real,
    width integer,
    height integer,
    encoding string,
    analog_gain real,
    awb_gain_red real,
    awb_gain_blue real,
    awb_mode string,
    brightness integer,
    contrast integer,
    digital_gain real,
    drc_strength string,
    exposure_compensation integer,
    exposure_mode string,
    shutter_speed integer,
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
    roi box, -- AKA zoom
);
